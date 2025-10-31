#include "mainwindow.h"

#include <QApplication>
#include <QStatusBar>

#include "CenterStack.h"
#include "LogManager.h"
#include "ModifyPage.h"
#include "NavDock.h"
#include "ShortcutManager.h"
#include "TopMenu.h"

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    resize(1200, 800);

    // 최소만 먼저
    ensureCenter();  // 중앙 스택만 우선
    ensureMenu();    // 상단 메뉴(Show Log 서브메뉴 준비)
    ensureLog();     // 로그(도킹/임베드 제어)
    ensureNav();     // 좌측 네비

    wire();
    statusBar()->showMessage("UI Ready (modular)");
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
        // TopMenu에서 액션/그룹을 받아 LogManager에 주입
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
    // 왼쪽 네비 → 페이지 전환
    connect(nav_.get(), &NavDock::clickCompare, this, [=] { center_->showCompare(); });
    connect(nav_.get(), &NavDock::clickBackup, this, [=] { center_->showBackup(); });
    connect(nav_.get(), &NavDock::clickOpenFile, this, [=] { center_->showOpenFile(); });
    connect(nav_.get(), &NavDock::clickApply, this, [=] { center_->showApply(); });
    connect(nav_.get(), &NavDock::clickModify, this, [=] { center_->showModify(); });

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
    connect(center_.get(), &CenterStack::backupRequested, this, [=](const QString &T) {
        if (logm_)
            logm_->append(QString("[UI] Backup target=%1").arg(T));
        statusBar()->showMessage("Backup (stub)");
    });
}
