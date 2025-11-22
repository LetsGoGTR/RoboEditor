#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStyleHints>

#include "ApplyPage.h"
#include "CenterStack.h"
#include "LogManager.h"
#include "ModifyPage.h"
#include "NavDock.h"
#include "ShortcutManager.h"
#include "TopMenu.h"

bool MainWindow::dark = false;
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

    wire();

    qApp->installEventFilter(this);

    applyStyleSheet();

    statusBar()->showMessage("UI Ready");
}

void MainWindow::ensureMenu()
{
    if (!menu_)
        menu_ = std::make_unique<TopMenu>(this);
    connect(menu_.get(), &TopMenu::requestModifyMenuUpdate, this, &MainWindow::populateModifyMenu);
    connect(menu_.get(), &TopMenu::requestRemoveMenuUpdate, this, &MainWindow::populateRemoveMenu);
    connect(menu_.get(), &TopMenu::fullScreenRequested, this, &MainWindow::setFullScreen);
    connect(menu_.get(), &TopMenu::maximizeRequested, this, &MainWindow::setMaximize);
}

void MainWindow::ensureCenter()
{
    if (!center_) {
        center_ = std::make_unique<CenterStack>(this);
        setCentralWidget(center_.get());
    }
}

void MainWindow::ensureLog()
{
    if (!menu_)
        return;

    // 싱글톤 방식으로 초기화
    LogManager::initialize(
            this, menu_->logVisibleAction(), menu_->logPosGroup(), menu_->showLogParentAction());

    // LogManager의 Dock을 숨기기 (CenterStack에서 표시하므로)
    if (LogManager::instance() && LogManager::instance()->dock()) {
        LogManager::instance()->dock()->hide();
    }

    // CenterStack과 연결
    if (center_) {
        center_->connectLogManager();
    }
}
void MainWindow::wire()
{
    if (!center_) {
        qDebug() << "[wire] Some component is null!"
                 << "center=" << center_.get();
        return;
    }

    modifyPage  = center_->getModifyPage();
    comparePage = center_->getComparePage();

    //ShortCutManager -> Qt의 setShortcut 사용
    //shortcutMgr = new ShortcutManager(this);
    //shortcutMgr->registerTo(this);

    // connect(shortcutMgr, &ShortcutManager::openRequested, modifyPage, &ModifyPage::openFile);
    // connect(shortcutMgr, &ShortcutManager::saveRequested, modifyPage, &ModifyPage::saveFile);
    // connect(shortcutMgr, &ShortcutManager::saveAsRequested, this, [this]() {
    //     modifyPage->saveAsFile();
    // });
    // connect(shortcutMgr,
    //         &ShortcutManager::closeRequested,
    //         modifyPage,
    //         &ModifyPage::closeCurrentTab);
    // connect(shortcutMgr, &ShortcutManager::quitRequested, this, []() { QApplication::quit(); });

    connect(center_.get(),
            &CenterStack::compareRequested,
            this,
            [=](const QString &L, const QString &R) {
                LogManager::append(QString("[UI] Compare: %1 | %2").arg(L, R));
                statusBar()->showMessage("Compare (stub)");
            });
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange) {
        qDebug() << "System theme changed → reloading stylesheet...";
        applyStyleSheet();
        return true;  // 이벤트 처리됐다고 알려서 재귀 방지
    }

    return QMainWindow::eventFilter(obj, event);
}
void MainWindow::applyStyleSheet()
{
    dark = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;

    QString stylePath = dark ? ":/styles/dark.qss" : ":/styles/light.qss";

    QFile f(stylePath);
    if (f.open(QFile::ReadOnly)) {
        QString css = QString::fromUtf8(f.readAll());
        qApp->setStyleSheet(css);
        qDebug() << " Style applied:" << stylePath;
    } else {
        qWarning() << " Failed to load stylesheet:" << stylePath;
    }
}
void MainWindow::toggleTheme()
{
    dark = !dark;

    QString stylePath = dark ? ":/styles/dark.qss" : ":/styles/light.qss";

    QFile f(stylePath);
    if (f.open(QFile::ReadOnly)) {
        QString css = QString::fromUtf8(f.readAll());
        qApp->setStyleSheet(css);
        qDebug() << "User toggled theme:" << (dark ? "Dark" : "Light");
    }
}

void MainWindow::createNewDocument()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->createNewFile();

        LogManager::append("New document created: untitled");
    }
}
void MainWindow::openFileFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->openFile();
    }
}
void MainWindow::saveFileFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->saveFile();
    }
}
void MainWindow::saveAsFileFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->saveAsFile();
    }
}
void MainWindow::saveAllFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->saveAll();
    }
}
void MainWindow::closeFileFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->closeFile();
    }
}
void MainWindow::closeAllFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        modify->closeAll();
    }
}
void MainWindow::undoFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->undo();
        }
    }
}
void MainWindow::redoFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->redo();
        }
    }
}
void MainWindow::cutFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->cut();
        }
    }
}
void MainWindow::copyFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->copy();
        }
    }
}
void MainWindow::pasteFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->paste();
        }
    }
}
void MainWindow::selectAllFromMenu()
{
    if (center_) {
        auto *modify = center_->getModifyPage();
        if (auto *ed = modify->getEditor()) {
            ed->selectAll();
            ;
        }
    }
}
void MainWindow::addCtrlFromMenu()
{
    ControllerManager::instance()->registerController();
    statusBar()->showMessage("Controller registration dialog opened");
}

ControllerManager *MainWindow::getControllerManager()
{
    return ControllerManager::instance();
}
void MainWindow::refreshCtrlFromMenu()
{
    ControllerManager::instance()->updateControllersStates();
    statusBar()->showMessage("Controller list refreshed");
}
void MainWindow::populateModifyMenu(QMenu *menu)
{
    if (!menu)
        return;

    menu->clear();

    //컨트롤러 매니저 가져오기
    auto *mgr = getControllerManager();
    if (!mgr)
        return;

    //컨트롤러 리스트 가져오기
    auto list = mgr->getControllers();

    if (list.isEmpty()) {
        menu->addAction("(No Controllers)")->setEnabled(false);
        return;
    }

    // action 추가, 발생 시 수정함수 호출
    for (const auto &info : list) {
        QAction *a = menu->addAction(info.serialNumber);

        // ControllerManager::modifyController 직접 호출
        connect(a, &QAction::triggered, this, [mgr, info]() { mgr->updateInfo(info); });
    }
}

void MainWindow::populateRemoveMenu(QMenu *menu)
{
    if (!menu)
        return;

    menu->clear();

    auto *mgr = getControllerManager();
    if (!mgr)
        return;

    auto list = mgr->getControllers();

    if (list.isEmpty()) {
        menu->addAction("(No Controllers)")->setEnabled(false);
        return;
    }

    for (const auto &info : list) {
        QString  serial = info.serialNumber;
        QAction *a      = menu->addAction(serial);

        connect(a, &QAction::triggered, this, [this, mgr, serial]() {
            QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Remove Controller",
                    QString("Are you sure you want to remove controller [%1]?").arg(serial),
                    QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                mgr->removeControllerBySN(serial);
            }
        });
    }
}
void MainWindow::setFullScreen(bool enable)
{
    if (enable) {
        // 풀스크린 진입
        isFullScreen_ = true;
        showFullScreen();
        statusBar()->showMessage("Full Screen Mode", 2000);
        LogManager::append("Entered Full Screen mode");
    } else {
        // 풀스크린 해제
        isFullScreen_ = false;
        showNormal();
        statusBar()->showMessage("Exited Full Screen Mode", 2000);
        LogManager::append("Exited Full Screen mode");
    }
}

void MainWindow::setMaximize(bool enable)
{
    if (enable) {
        // 최대화
        if (isFullScreen_) {
            // 풀스크린이면 먼저 해제
            showNormal();
            isFullScreen_ = false;
        }
        showMaximized();
        statusBar()->showMessage("Window Maximized", 2000);
        LogManager::append("Window maximized");
    } else {
        // 일반 크기로
        showNormal();
        statusBar()->showMessage("Window Restored", 2000);
        LogManager::append("Window restored to normal size");
    }
}

void MainWindow::compareFileFromMenu()
{
    center_->showModifyWithCompareFile();
}
void MainWindow::compareFolderFromMenu()
{
    center_->showModifyWithCompareFolders();
}
void MainWindow::backupFromMenu()
{
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
    connect(backupPopup_, &QWidget::destroyed, this, [this]() { backupPopup_ = nullptr; });
    backupPopup_->show();
}
void MainWindow::applyFromMenu()
{
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

    QObject::connect(applyPopup_, &QWidget::destroyed, this, [this]() { applyPopup_ = nullptr; });

    applyPopup_->show();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
}
