#include "TopMenu.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include "ControllerManager.h"
#include "LogManager.h"
#include "PasswordManager.h"
#include "mainwindow.h"

TopMenu::TopMenu(MainWindow *mw) : QObject(mw), mw_(mw)
{
    build();

    qDebug() << "TopMenu good";
}

void TopMenu::build()
{
    auto *mb = mw_->menuBar();
    file_    = mb->addMenu("File");
    edit_    = mb->addMenu("Edit");
    ctrl_    = mb->addMenu("Controllers");
    view_    = mb->addMenu("View");
    tools_   = mb->addMenu("Tools");
    help_    = mb->addMenu("Help");

    // add file actions
    actNewFile_  = file_->addAction("New File");
    actOpenFile_ = file_->addAction("Open File");
    file_->addSeparator();
    actSaveFile_   = file_->addAction("Save File");
    actSaveAsFile_ = file_->addAction("Save as File");
    actSaveAll_    = file_->addAction("Save All");
    file_->addSeparator();
    actCloseFile_ = file_->addAction("Close File");
    actCloseAll_  = file_->addAction("Close All");
    file_->addSeparator();
    actExit_ = file_->addAction("Exit");

    actNewFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    actOpenFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    actSaveFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    actSaveAll_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    actCloseFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    actCloseAll_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    actExit_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));

    connect(actNewFile_, &QAction::triggered, mw_, &MainWindow::createNewDocument);
    connect(actOpenFile_, &QAction::triggered, mw_, &MainWindow::openFileFromMenu);
    connect(actSaveFile_, &QAction::triggered, mw_, &MainWindow::saveFileFromMenu);
    connect(actSaveAsFile_, &QAction::triggered, mw_, &MainWindow::saveAsFileFromMenu);
    connect(actSaveAll_, &QAction::triggered, mw_, &MainWindow::saveAllFromMenu);
    connect(actCloseFile_, &QAction::triggered, mw_, &MainWindow::closeFileFromMenu);
    connect(actCloseAll_, &QAction::triggered, mw_, &MainWindow::closeAllFromMenu);
    connect(actExit_, &QAction::triggered, qApp, &QApplication::quit);

    // add edit actions

    actUndo_ = edit_->addAction("Undo");
    actRedo_ = edit_->addAction("Redo");
    edit_->addSeparator();
    actCut_   = edit_->addAction("Cut");
    actCopy_  = edit_->addAction("Copy");
    actPaste_ = edit_->addAction("Paste");
    edit_->addSeparator();
    actSelectAll_ = edit_->addAction("Select All");

    actUndo_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    actRedo_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
    actCut_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X));
    actCopy_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    actPaste_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    actSelectAll_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));

    connect(actUndo_, &QAction::triggered, mw_, &MainWindow::undoFromMenu);
    connect(actRedo_, &QAction::triggered, mw_, &MainWindow::redoFromMenu);
    connect(actCut_, &QAction::triggered, mw_, &MainWindow::cutFromMenu);
    connect(actCopy_, &QAction::triggered, mw_, &MainWindow::copyFromMenu);
    connect(actPaste_, &QAction::triggered, mw_, &MainWindow::pasteFromMenu);
    connect(actSelectAll_, &QAction::triggered, mw_, &MainWindow::selectAllFromMenu);

    // add controllers actions

    actAddController_ = ctrl_->addAction("Add Controller");
    modifyMenu_       = ctrl_->addMenu("Modify Controller Setting");
    removeMenu_       = ctrl_->addMenu("Remove Controller");
    actRefreshList_   = ctrl_->addAction("Refresh List");
    ctrl_->addSeparator();
    actCompareFile_   = ctrl_->addAction("Compare Files");
    actCompareFolder_ = ctrl_->addAction("Compare Folders");
    actBackup_        = ctrl_->addAction("Backup from Controller");
    actApply_         = ctrl_->addAction("Apply to Controller");

    connect(actAddController_, &QAction::triggered, mw_, &MainWindow::addCtrlFromMenu);
    connect(actRefreshList_, &QAction::triggered, mw_, &MainWindow::refreshCtrlFromMenu);
    connect(modifyMenu_, &QMenu::aboutToShow, this, [this]() {
        emit requestModifyMenuUpdate(modifyMenu_);
    });

    connect(removeMenu_, &QMenu::aboutToShow, this, [this]() {
        emit requestRemoveMenuUpdate(removeMenu_);
    });

    connect(actCompareFile_, &QAction::triggered, mw_, &MainWindow::compareFileFromMenu);
    connect(actCompareFolder_, &QAction::triggered, mw_, &MainWindow::compareFolderFromMenu);
    connect(actBackup_, &QAction::triggered, mw_, &MainWindow::backupFromMenu);
    connect(actApply_, &QAction::triggered, mw_, &MainWindow::applyFromMenu);

    actRefreshList_->setShortcut(QKeySequence(Qt::Key_F5));
    actBackup_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    actAddController_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    actCompareFile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    actCompareFolder_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));

    // add view actions

    actToggleLog_ = new QAction("Show Log", this);
    actToggleLog_->setCheckable(true);
    actToggleLog_->setChecked(true);

    showLogMenu_ = view_->addMenu("Log Position");
    posGroup_    = new QActionGroup(this);
    posGroup_->setExclusive(true);

    actLogPanel_  = mk("Log → In Editor");
    actLogBottom_ = mk("Log → Bottom");
    actLogRight_  = mk("Log → Right");
    actLogLeft_   = mk("Log → Left");
    actLogTop_    = mk("Log → Top");

    actLogPanel_->setChecked(true);

    view_->addAction(actToggleLog_);
    view_->addMenu(showLogMenu_);

    // 시그널 연결
    connect(actToggleLog_, &QAction::toggled, this, &TopMenu::logToggled);

    connect(posGroup_, &QActionGroup::triggered, this, [this](QAction *act) {
        if (act == actLogPanel_) {
            emit logPositionChanged(Qt::NoDockWidgetArea, true);
        } else if (act == actLogBottom_) {
            emit logPositionChanged(Qt::BottomDockWidgetArea, false);
        } else if (act == actLogRight_) {
            emit logPositionChanged(Qt::RightDockWidgetArea, false);
        } else if (act == actLogLeft_) {
            emit logPositionChanged(Qt::LeftDockWidgetArea, false);
        } else if (act == actLogTop_) {
            emit logPositionChanged(Qt::TopDockWidgetArea, false);
        }
    });

    screenGroup_ = new QActionGroup(this);
    auto sS      = [&](const char *t) {
        auto *a = new QAction(t, this);
        a->setCheckable(true);
        screenGroup_->addAction(a);
        return a;
    };

    setScreenSize_ = view_->addMenu("Screen Size");

    actFullScreen_ = sS("Full Screen");
    actFullScreen_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F11));
    actMaximize_ = sS("MaxiMize");
    actMaximize_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F11));

    setScreenSize_->addAction(actFullScreen_);
    setScreenSize_->addAction(actMaximize_);

    connect(actFullScreen_, &QAction::toggled, this, [this](bool checked) {
        emit fullScreenRequested(checked);
    });

    connect(actMaximize_, &QAction::toggled, this, [this](bool checked) {
        emit maximizeRequested(checked);
    });

    // add tools actions
    actChangePswd_ = new QAction("Change Password", this);

    connect(actChangePswd_, &QAction::triggered, this, &TopMenu::onChangePasswordTriggered);
    tools_->addAction(actChangePswd_);

    // add help actions

    actUserGuide_ = help_->addAction("User Guide");
    actUserGuide_->setShortcut(QKeySequence(Qt::Key_F1));
    actShortcuts_ = help_->addAction("Keyboard Shortcuts");
    help_->addSeparator();
    actSystemInfo_ = help_->addAction("System Information");
    help_->addSeparator();
    actAbout_ = help_->addAction("About RoboEditor");

    connect(actUserGuide_, &QAction::triggered, mw_, &MainWindow::showUserGuideFromMenu);
    connect(actShortcuts_, &QAction::triggered, mw_, &MainWindow::showShortcutsFromMenu);
    connect(actSystemInfo_, &QAction::triggered, mw_, &MainWindow::showSystemInfoFromMenu);
    connect(actAbout_, &QAction::triggered, mw_, &MainWindow::showAboutFromMenu);

    //themeToggle_ = new QAction("Toggle Dark/Light", this);
    //sett_->addAction(themeToggle_);

    // add help actions
    qDebug() << "insert done";
}

void TopMenu::onChangePasswordTriggered()
{
    PasswordManager manager(mw_);

    QObject::connect(&manager,
                     &PasswordManager::masterPasswordChanged,
                     ControllerManager::instance(),
                     &ControllerManager::onMasterPasswordChanged);

    manager.changePassword();
}
QAction *TopMenu::mk(const QString &text)
{
    QAction *act = new QAction(text, this);
    act->setCheckable(true);
    showLogMenu_->addAction(act);
    posGroup_->addAction(act);
    return act;
}
