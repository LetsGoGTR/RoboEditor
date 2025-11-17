#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>

#include "ApplyPage.h"
#include "CenterStack.h"
#include "LogManager.h"
#include "ModifyPage.h"
#include "NavDock.h"
#include "ShortcutManager.h"
#include "TopMenu.h"

MainWindow::~MainWindow()
{
    LogManager::destroy();
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 컴포넌트 초기화
    ensureCenter();  // 중앙 위젯 (파일 트리 + 에디터)
    ensureMenu();    // 상단 메뉴
    ensureLog();     // 로그 관리자
    ensureNav();     // 좌측 네비게이션

    wire();

    statusBar()->showMessage("UI Ready");
}

void MainWindow::ensureMenu()
{
    if (!menu_)
        menu_ = std::make_unique<TopMenu>(this);
}

void MainWindow::ensureCenter()
{
    if (!center_) {
        center_ = std::make_unique<CenterStack>(this);
        setCentralWidget(center_.get());
    }
}

void MainWindow::ensureNav()
{
    if (!nav_) {
        nav_ = std::make_unique<NavDock>(this);
        addToolBar(Qt::TopToolBarArea, nav_.get());
    }
}

void MainWindow::ensureLog()
{
    if (!menu_)
        return;
    // 싱글톤 방식으로 초기화
    LogManager::initialize(
            this, menu_->logVisibleAction(), menu_->logPosGroup(), menu_->showLogParentAction());
}
void MainWindow::wire()
{
    if (!nav_ || !center_) {
        qDebug() << "[wire] Some component is null!"
                 << "nav=" << nav_.get() << "center=" << center_.get();
        return;
    }

    // 상단 Nav UI 전환 연결
    connect(nav_.get(), &NavDock::clickCompare, center_.get(), &CenterStack::showModifyWithCompare);

    // Nav 기능 -> Pop-up
    connect(nav_.get(), &NavDock::clickApply, this, [this]() {
        if (applyPopup_ && applyPopup_->isVisible()) {
            applyPopup_->raise();
            applyPopup_->activateWindow();
            return;
        }

        applyPopup_ = new QWidget(nullptr, Qt::Window);
        applyPopup_->setAttribute(Qt::WA_DeleteOnClose);
        applyPopup_->setWindowTitle("Apply to Robot Controller");
        applyPopup_->resize(900, 600);

        auto *applyPage = new ApplyPage(applyPopup_);
        auto *layout    = new QVBoxLayout(applyPopup_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(applyPage);

        QObject::connect(
                applyPopup_, &QWidget::destroyed, this, [this]() { applyPopup_ = nullptr; });

        applyPopup_->show();
    });

    connect(nav_.get(), &NavDock::clickBackup, this, [this]() {
        if (backupPopup_ && backupPopup_->isVisible()) {
            backupPopup_->raise();
            backupPopup_->activateWindow();
            return;
        }

        backupPopup_ = new QWidget(nullptr, Qt::Window);
        backupPopup_->setAttribute(Qt::WA_DeleteOnClose);
        backupPopup_->setWindowTitle("Backup from Robot Controller");
        backupPopup_->resize(900, 600);

        auto *backupPage = new BackupPage(backupPopup_);
        auto *layout     = new QVBoxLayout(backupPopup_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(backupPage);

        QObject::connect(
                backupPopup_, &QWidget::destroyed, this, [this]() { backupPopup_ = nullptr; });

        backupPopup_->show();
    });

    modifyPage = center_->getModifyPage();

    shortcutMgr = new ShortcutManager(this);
    shortcutMgr->registerTo(this);

    connect(shortcutMgr, &ShortcutManager::openRequested, modifyPage, &ModifyPage::openFile);
    connect(shortcutMgr, &ShortcutManager::saveRequested, modifyPage, &ModifyPage::saveFile);
    connect(shortcutMgr, &ShortcutManager::saveAsRequested, modifyPage, &ModifyPage::saveAsFile);
    connect(shortcutMgr,
            &ShortcutManager::closeRequested,
            modifyPage,
            &ModifyPage::closeCurrentTab);
    connect(shortcutMgr, &ShortcutManager::quitRequested, this, []() { QApplication::quit(); });

    connect(center_.get(),
            &CenterStack::compareRequested,
            this,
            [=](const QString &L, const QString &R) {
                LogManager::append(QString("[UI] Compare: %1 | %2").arg(L, R));
                statusBar()->showMessage("Compare (stub)");
            });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
}
