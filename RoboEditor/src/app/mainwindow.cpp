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

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 컴포넌트 초기화
    ensureCenter();  // 중앙 위젯 (파일 트리 + 에디터)
    ensureMenu();    // 상단 메뉴
    ensureLog();     // 로그 관리자
    ensureNav();     // 좌측 네비게이션

    wire();

    // 이전 세션의 설정 복원
    //loadSettings();

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
    if (!logm_) {
        logm_ = std::make_unique<LogManager>(this,
                                             menu_->logVisibleAction(),
                                             menu_->logPosGroup(),
                                             menu_->showLogParentAction());
    }
}

void MainWindow::wire()
{
    if (!nav_ || !center_ || !logm_) {
        qDebug() << "[wire] Some component is null!"
                 << "nav=" << nav_.get() << "center=" << center_.get() << "logm=" << logm_.get();
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

        // 닫힐 때 포인터 초기화
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

        // 닫힐 때 포인터 초기화
        QObject::connect(
                backupPopup_, &QWidget::destroyed, this, [this]() { backupPopup_ = nullptr; });

        backupPopup_->show();
    });

    modifyPage = center_->getModifyPage();

    // 단축키 manager 등록
    shortcutMgr = new ShortcutManager(this);
    shortcutMgr->registerTo(this);

    // 단축키 mapping
    connect(shortcutMgr, &ShortcutManager::openRequested, modifyPage, &ModifyPage::openFile);
    connect(shortcutMgr, &ShortcutManager::saveRequested, modifyPage, &ModifyPage::saveFile);
    connect(shortcutMgr, &ShortcutManager::saveAsRequested, modifyPage, &ModifyPage::saveAsFile);
    connect(shortcutMgr,
            &ShortcutManager::closeRequested,
            modifyPage,
            &ModifyPage::closeCurrentTab);
    connect(shortcutMgr, &ShortcutManager::quitRequested, this, []() { QApplication::quit(); });

    // 그 외 기능 추가
    connect(center_.get(),
            &CenterStack::compareRequested,
            this,
            [=](const QString &L, const QString &R) {
                if (logm_)
                    logm_->append(QString("[UI] Compare: %1 | %2").arg(L, R));
                statusBar()->showMessage("Compare (stub)");
            });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSettings()
{
    // config 폴더 경로 설정
    QString configDir = "C:/backup/config";
    QDir    dir;
    if (!dir.exists(configDir)) {
        dir.mkpath(configDir);
    }

    QString   configFile = configDir + "/settings.ini";
    QSettings settings(configFile, QSettings::IniFormat);

    // 윈도우 크기와 위치 저장
    settings.setValue("MainWindow/geometry", saveGeometry());

    // 독 위젯과 툴바 상태 저장
    settings.setValue("MainWindow/windowState", saveState());

    // CenterStack의 Splitter 상태 저장 (좌측 트리와 우측 에디터의 비율)
    if (center_) {
        QByteArray splitterState = center_->saveSplitterState();
        settings.setValue("CenterStack/splitterState", splitterState);

        // ModifyPage의 Splitter 상태 저장 (에디터와 Compare 패널 비율)
        // ComparePage가 열려있을 때만 저장 (ComparePage가 없으면 복원 시 충돌 발생)
        ModifyPage *modifyPage = center_->getModifyPage();
        if (modifyPage) {
            // ComparePage가 열려있는지 확인하고 저장
            modifyPage
                    ->saveComparePageState();  // 내부에서 ComparePage 상태와 ModifyPage 상태 모두 저장
        }
    }

    settings.sync();  // 파일에 즉시 쓰기
}

void MainWindow::loadSettings()
{
    QString configFile = "C:/backup/config/settings.ini";

    // 설정 파일이 없으면 복원하지 않음
    if (!QFile::exists(configFile)) {
        return;
    }

    QSettings settings(configFile, QSettings::IniFormat);

    // 윈도우 크기와 위치 복원
    if (settings.contains("MainWindow/geometry")) {
        restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    }

    // 독 위젯과 툴바 상태 복원
    if (settings.contains("MainWindow/windowState")) {
        restoreState(settings.value("MainWindow/windowState").toByteArray());
    }

    // CenterStack의 Splitter 상태 복원
    if (center_ && settings.contains("CenterStack/splitterState")) {
        center_->restoreSplitterState(settings.value("CenterStack/splitterState").toByteArray());
    }

    // ModifyPage의 Splitter 상태 복원은 ComparePage 생성 시에만 수행
    // (ComparePage가 없을 때 복원하면 ComparePage 생성 시 충돌 발생)
    // ModifyPage의 기본 splitter 상태는 ComparePage 없을 때의 상태이므로
    // ComparePage가 열릴 때 자동으로 복원됨
}
