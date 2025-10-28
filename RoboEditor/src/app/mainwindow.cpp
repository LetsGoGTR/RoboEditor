#include "mainwindow.h"
#include "TopMenu.h"
#include "NavDock.h"
#include "CenterStack.h"
#include "LogManager.h"

#include <QApplication>
#include <QStatusBar>

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent) {
    resize(1200,800);

    ensureCenter();   // 중앙 스택
    ensureMenu();     // 상단 메뉴(Show Log 서브메뉴 준비)
    ensureLog();      // 로그(도킹/임베드 제어)
    ensureNav();      // 좌측 네비

    wire();
    statusBar()->showMessage("UI Ready (modular)");
}

void MainWindow::ensureMenu(){
    if(!menu_) menu_ = std::make_unique<TopMenu>(this);
}
void MainWindow::ensureCenter(){
    if(!center_) center_ = std::make_unique<CenterStack>(this);
}
void MainWindow::ensureNav(){
    if(!nav_) nav_ = std::make_unique<NavDock>(this);
}
void MainWindow::ensureLog(){
    if (!menu_) return;
    if(!logm_){
        // TopMenu에서 액션/그룹을 받아 LogManager에 주입
        logm_ = std::make_unique<LogManager>(
                this,
                menu_->logVisibleAction(),
                menu_->logPosGroup(),
                menu_->showLogParentAction()
                );
    }
}

void MainWindow::wire() {
    if (!nav_ || !center_ || !logm_) {
        qDebug() << "[wire] Some component is null!"
                 << "nav="    << nav_.get()
                 << "center=" << center_.get()
                 << "logm="   << logm_.get();
        return;
    }

    // 좌측 네비 버튼 → CenterStack의 페이지 전환
    connect(nav_.get(), &NavDock::clickCompare, this, [=]{
        center_->showCompare(); // ComparePage로 이동
    });
    connect(nav_.get(), &NavDock::clickBackup,  this, [=]{
        center_->showBackup();
    });

    // ComparePage / BackupPage에서 온 이벤트 -> 로그 찍어주기
    connect(center_.get(), &CenterStack::compareRequested, this,
            [=](const QString& L, const QString& R){
                if (logm_) logm_->append(QString("[UI] Compare request: %1 | %2").arg(L, R));
                statusBar()->showMessage("Compare requested");
            });
    connect(center_.get(), &CenterStack::backupRequested, this,
            [=](const QString& T){
                if (logm_) logm_->append(QString("[UI] Backup target=%1").arg(T));
                statusBar()->showMessage("Backup requested");
            });

}
