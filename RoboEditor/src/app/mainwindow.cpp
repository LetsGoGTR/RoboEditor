#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStyleHints>
#include <QSysInfo>
#include <QUrl>

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

    ensureMenu();    // 상단 메뉴
    ensureLog();     // 로그 관리자
    ensureCenter();  // 중앙 위젯 (파일 트리 + 에디터)
    wire();

    qDebug() << "mainwindow good";
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
    qDebug() << "[ensureCenter] START";

    if (!center_) {
        qDebug() << "[ensureCenter] Creating CenterStack...";
        center_ = std::make_unique<CenterStack>(this);
        qDebug() << "[ensureCenter] CenterStack created!";

        qDebug() << "[ensureCenter] Setting central widget...";
        setCentralWidget(center_.get());
        qDebug() << "[ensureCenter] Central widget set!";
    }

    qDebug() << "[ensureCenter] END";
}

void MainWindow::ensureLog()
{
    if (!menu_)
        return;

    LogManager::initialize();

    // TopMenu 시그널 연결
    connect(menu_.get(), &TopMenu::logToggled, this, &MainWindow::onLogToggled);
    connect(menu_.get(), &TopMenu::logPositionChanged, this, &MainWindow::onLogPositionChanged);

    m_logVisible = true;
    m_logIsPanel = true;
}
void MainWindow::wire()
{
    if (!center_) {
        qDebug() << "[wire] Some component is null!"
                 << "center=" << center_.get();
        return;
    }

    QString path = LogManager::getLogFilePath();
    QString msg  = QString("Log File Loaded From [%1]").arg(path);
    LogManager::append(msg);

    modifyPage  = center_->getModifyPage();
    comparePage = center_->getComparePage();

    updateLogView();
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
void MainWindow::updateLogView()
{
    // 1. 숨김
    if (!m_logVisible) {
        center_->hideLogPanel();
        if (m_logDock)
            m_logDock->hide();
        return;
    }

    // 2. Panel 모드
    if (m_logIsPanel) {
        // Dock 정리 (MainWindow가 직접 관리)
        if (m_logDock) {
            removeDockWidget(m_logDock);
            m_logDock->deleteLater();
            m_logDock = nullptr;
        }

        // Panel 표시 (CenterStack에 위임)
        center_->showLogPanel();
        return;
    }

    // 3. Dock 모드
    center_->hideLogPanel();

    if (!m_logDock) {
        m_logDock = new QDockWidget(this);
        m_logDock->setObjectName("LogDock");
        m_logDock->setAllowedAreas(Qt::AllDockWidgetAreas);
        m_logDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                               QDockWidget::DockWidgetClosable);

        QFrame *header = new QFrame;
        header->setObjectName("LogHeader");
        header->setFrameShape(QFrame::NoFrame);
        QHBoxLayout *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(8, 6, 8, 6);

        QLabel *titleLabel = new QLabel;
        titleLabel->setObjectName("LogTitle");
        titleLabel->setText(LogManager::instance()->getLogFileName());
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        m_logDock->setTitleBarWidget(header);

        QPlainTextEdit *logView = new QPlainTextEdit;
        logView->setReadOnly(true);
        logView->setMaximumBlockCount(1000);
        logView->setObjectName("LogView");

        connect(LogManager::instance(),
                &LogManager::logAppended,
                logView,
                [logView](const QString &text) {
                    logView->appendPlainText(text);
                    QTextCursor cursor = logView->textCursor();
                    cursor.movePosition(QTextCursor::End);
                    logView->setTextCursor(cursor);
                });

        m_logDock->setWidget(logView);
    }

    addDockWidget(m_logArea, m_logDock);
    m_logDock->show();
}
void MainWindow::onLogToggled(bool visible)
{
    m_logVisible = visible;
    updateLogView();

    QString msg = visible ? "Log shown" : "Log hidden";
    LogManager::append(msg);
}

void MainWindow::onLogPositionChanged(Qt::DockWidgetArea area, bool isPanel)
{
    m_logArea    = area;
    m_logIsPanel = isPanel;
    updateLogView();

    QString pos;
    if (isPanel) {
        pos = "In Editor";
    } else {
        switch (area) {
        case Qt::BottomDockWidgetArea:
            pos = "Bottom Dock";
            break;
        case Qt::TopDockWidgetArea:
            pos = "Top Dock";
            break;
        case Qt::LeftDockWidgetArea:
            pos = "Left Dock";
            break;
        case Qt::RightDockWidgetArea:
            pos = "Right Dock";
            break;
        default:
            pos = "Floating Dock";
            break;
        }
    }
    LogManager::append(QString("Log position: %1").arg(pos));
}

void MainWindow::showShortcutsFromMenu()
{
    QString shortcuts = "<h3>File Operations</h3>"
                        "Ctrl+N - New File<br>"
                        "Ctrl+O - Open File<br>"
                        "Ctrl+S - Save File<br>"
                        "Ctrl+Shift+S - Save All<br>"
                        "Ctrl+W - Close File<br>"
                        "Ctrl+Shift+W - Close All<br>"
                        "Ctrl+Q - Exit<br>"
                        "<br>"
                        "<h3>Edit Operations</h3>"
                        "Ctrl+Z - Undo<br>"
                        "Ctrl+Y - Redo<br>"
                        "Ctrl+X - Cut<br>"
                        "Ctrl+C - Copy<br>"
                        "Ctrl+V - Paste<br>"
                        "Ctrl+A - Select All<br>"
                        "<br>"
                        "<h3>Controller Operations</h3>"
                        "Ctrl+Shift+N - Add Controller<br>"
                        "F5 - Refresh List<br>"
                        "Ctrl+D - Compare Files<br>"
                        "Ctrl+Shift+D - Compare Folders<br>"
                        "Ctrl+B - Backup from Controller<br>"
                        "<br>"
                        "<h3>Help</h3>"
                        "F1 - User Guide<br>";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Keyboard Shortcuts");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(shortcuts);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void MainWindow::showSystemInfoFromMenu()
{
    QString info = QString("<b>RoboEditor System Information</b><br><br>"
                           "<b>Version:</b> 1.0.0<br>"
                           "<b>Qt Version:</b> %1<br>"
                           "<b>Build Date:</b> %2<br>"
                           "<b>Operating System:</b> %3<br>"
                           "<b>Architecture:</b> %4")
                           .arg(qVersion())
                           .arg(__DATE__)
                           .arg(QSysInfo::prettyProductName())
                           .arg(QSysInfo::currentCpuArchitecture());

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("System Information");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(info);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}
void MainWindow::showUserGuideFromMenu()
{
    QString guide = "<h2>RoboEditor User Guide</h2>"
                    "<h3>Getting Started</h3>"
                    "<p><b>Adding a Controller:</b><br>"
                    "1. Click 'Add Controller' or press Ctrl+Shift+N<br>"
                    "2. Enter controller name, IP address, and credentials<br>"
                    "3. Test connection and save configuration</p>"
                    "<br>"
                    "<h3>File Operations</h3>"
                    "<p><b>Opening Files:</b><br>"
                    "- Use Ctrl+O to open local files<br>"
                    "- Double-click controller files in the tree view to open remotely</p>"
                    "<p><b>Editing Files:</b><br>"
                    "- Syntax highlighting is automatically applied<br>"
                    "- Changes are saved locally until you upload to controller</p>"
                    "<p><b>Saving Files:</b><br>"
                    "- Ctrl+S: Save current file<br>"
                    "- Ctrl+Shift+S: Save all open files</p>"
                    "<br>"
                    "<h3>Controller Management</h3>"
                    "<p><b>Backup/Restore:</b><br>"
                    "- Right-click controller → 'Backup from Controller'<br>"
                    "- Backups are stored as compressed archives with timestamps<br>"
                    "- Use 'Restore to Controller' to upload backup files</p>"
                    "<p><b>File Comparison:</b><br>"
                    "- Ctrl+D: Compare two files side-by-side<br>"
                    "- Ctrl+Shift+D: Compare entire folders with diff highlighting</p>"
                    "<p><b>SFTP Operations:</b><br>"
                    "- Upload/Download files via right-click context menu<br>"
                    "- Browse remote filesystem in tree view<br>"
                    "- Monitor transfer progress in status bar</p>"
                    "<br>"
                    "<h3>Logs & Monitoring</h3>"
                    "<p><b>Log Panel:</b><br>"
                    "- View real-time system logs in bottom panel<br>"
                    "- Filter by log level (Debug/Info/Warning/Error)<br>"
                    "- Export logs for debugging purposes</p>"
                    "<br>"
                    "<h3>Tips & Tricks</h3>"
                    "<p>• Use F5 to refresh controller file lists<br>"
                    "• Right-click tabs for quick file operations<br>"
                    "• Drag files between local and remote views<br>"
                    "• Use search (Ctrl+F) to find text in open files<br>"
                    "• Check status bar for connection status</p>"
                    "<br>"
                    "<h3>Troubleshooting</h3>"
                    "<p><b>Connection Issues:</b><br>"
                    "- Verify IP address and credentials<br>"
                    "- Check network connectivity<br>"
                    "- Ensure SSH/SFTP service is running on controller</p>"
                    "<p><b>File Transfer Errors:</b><br>"
                    "- Check file permissions on remote system<br>"
                    "- Verify sufficient disk space<br>"
                    "- Review logs for detailed error messages</p>"
                    "<br>"
                    "<p><i>For additional support, press F1 or contact technical support.</i></p>";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("User Guide");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(guide);
    msgBox.setIcon(QMessageBox::Information);

    // 내용이 길어서 스크롤 가능하도록 크기 조정
    msgBox.setStyleSheet("QMessageBox { min-width: 600px; }");

    msgBox.exec();
}
void MainWindow::showAboutFromMenu()
{
    QString aboutText = "<h2>RoboEditor</h2>"
                        "<p><b>Version 1.0.0</b></p>"
                        "<p>Robot Controller Management Tool</p>"
                        "<p>Developed in collaboration with<br>"
                        "Samsung Electronics Production Technology Research Institute</p>"
                        "<br>";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("About RoboEditor");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(aboutText);
    msgBox.setIconPixmap(QPixmap(":/icons/app_icon.png").scaled(64, 64, Qt::KeepAspectRatio));
    msgBox.exec();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
}
