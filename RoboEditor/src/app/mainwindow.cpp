#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QStatusBar>

#include "Center.h"
#include "LogManager.h"
#include "ModifyPage.h"
#include "NavDock.h"
#include "ShortcutManager.h"
#include "TopMenu.h"

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    resize(1200, 800);

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
        center_ = std::make_unique<Center>(this);
        setCentralWidget(center_.get());

        // Center 시그널 연결
        connect(center_.get(), &Center::fileOpened, this, [=](const QString &path) {
            if (logm_)
                logm_->append(QString("[Editor] Opened: %1").arg(path));
            statusBar()->showMessage(QString("Opened: %1").arg(path), 3000);
        });
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

    // 왼쪽 네비 → 페이지 전환

    // 네비게이션 버튼 → 다이얼로그/새 위젯 생성

    connect(nav_.get(), &NavDock::clickCompare, this, [=]() {
        if (logm_)
            logm_->append("[Nav] Compare clicked");

        // TODO: CompareDialog 구현 필요
        QMessageBox::information(this, "Compare", "Compare 기능 (구현 예정)");
        statusBar()->showMessage("Compare dialog");
    });

    connect(nav_.get(), &NavDock::clickBackup, this, [=]() {
        if (logm_)
            logm_->append("[Nav] Backup clicked");

        // TODO: BackupDialog 구현 필요
        QMessageBox::information(this, "Backup", "Backup 기능 (구현 예정)");
        statusBar()->showMessage("Backup dialog");
    });

    connect(nav_.get(), &NavDock::clickOpenFile, this, [=]() {
        if (logm_)
            logm_->append("[Nav] OpenFile clicked");

        // TODO: 파일 선택 다이얼로그
        // Center의 setRootPath() 호출하거나
        // 파일 다이얼로그로 특정 파일 열기
        QString path =
                QFileDialog::getExistingDirectory(this, "Select Directory", QDir::homePath());

        if (!path.isEmpty()) {
            center_->setBackupPath(path);
            statusBar()->showMessage(QString("Root changed: %1").arg(path));
        }
    });

    connect(nav_.get(), &NavDock::clickApply, this, [=]() {
        if (logm_)
            logm_->append("[Nav] Apply clicked");

        // TODO: ApplyDialog 구현 필요
        QMessageBox::information(this, "Apply", "Apply 기능 (구현 예정)");
        statusBar()->showMessage("Apply dialog");
    });

    connect(nav_.get(), &NavDock::clickModify, this, [=]() {
        if (logm_)
            logm_->append("[Nav] Modify clicked");

        // ModifyPage의 현재 Document 확인
        Document *currentDoc = center_->modifyPage()->currentDocument();
        if (!currentDoc) {
            QMessageBox::warning(this, "Modify", "열린 파일이 없습니다.");
        } else {
            QMessageBox::information(
                    this,
                    "Modify",
                    QString("Modify 기능 (구현 예정)\n현재 파일: %1").arg(currentDoc->gfilePath()));
        }
        statusBar()->showMessage("Modify dialog");
    });
}
