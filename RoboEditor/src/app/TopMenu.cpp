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
    actNewFile_    = file_->addAction("New File");
    actOpenFile_   = file_->addAction("Open File");
    actSaveFile_   = file_->addAction("Save File");
    actSaveAsFile_ = file_->addAction("Save as File");
    actSaveAll_    = file_->addAction("Save All");
    actCloseFile_  = file_->addAction("Close File");
    actCloseAll_   = file_->addAction("Close All");
    actExit_       = file_->addAction("Exit");

    connect(actNewFile_, &QAction::triggered, mw_, &MainWindow::createNewDocument);
    connect(actOpenFile_, &QAction::triggered, mw_, &MainWindow::openFileFromMenu);
    connect(actSaveFile_, &QAction::triggered, mw_, &MainWindow::saveFileFromMenu);
    connect(actSaveAsFile_, &QAction::triggered, mw_, &MainWindow::saveAsFileFromMenu);
    connect(actSaveAll_, &QAction::triggered, mw_, &MainWindow::saveAllFromMenu);
    connect(actCloseFile_, &QAction::triggered, mw_, &MainWindow::closeFileFromMenu);
    connect(actCloseAll_, &QAction::triggered, mw_, &MainWindow::closeAllFromMenu);
    connect(actExit_, &QAction::triggered, qApp, &QApplication::quit);

    // add edit actions

    actUndo_      = edit_->addAction("Undo");
    actRedo_      = edit_->addAction("Redo");
    actCut_       = edit_->addAction("Cut");
    actCopy_      = edit_->addAction("Copy");
    actPaste_     = edit_->addAction("Paste");
    actSelectAll_ = edit_->addAction("Select All");

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

    connect(actAddController_, &QAction::triggered, mw_, &MainWindow::addCtrlFromMenu);
    connect(actRefreshList_, &QAction::triggered, mw_, &MainWindow::refreshCtrlFromMenu);
    connect(modifyMenu_, &QMenu::aboutToShow, this, [this]() {
        emit requestModifyMenuUpdate(modifyMenu_);
    });

    connect(removeMenu_, &QMenu::aboutToShow, this, [this]() {
        emit requestRemoveMenuUpdate(removeMenu_);
    });

    // add view actions
    showLogMenu_ = view_->addMenu("Show Log");

    actToggleLog_ = new QAction("Show Log Viewer", this);
    actToggleLog_->setCheckable(true);
    actToggleLog_->setChecked(true);
    actToggleLog_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));

    showLogMenu_->addAction(actToggleLog_);
    showLogMenu_->addSeparator();

    posGroup_ = new QActionGroup(this);
    posGroup_->setExclusive(true);

    screenGroup_ = new QActionGroup(this);
    screenGroup_->setExclusive(true);

    auto mk = [&](const char *t) {
        auto *a = new QAction(t, this);
        a->setCheckable(true);
        posGroup_->addAction(a);
        return a;
    };

    showLogMenu_->addAction(mk("Log → Bottom"));
    showLogMenu_->addAction(mk("Log → Right"));
    showLogMenu_->addAction(mk("Log → Left"));
    showLogMenu_->addAction(mk("Log → Top"));
    showLogMenu_->addAction(mk("Log → Float"));
    posGroup_->actions().front()->setChecked(true);

    connect(actToggleLog_, &QAction::toggled, this, [this](bool checked) {
        if (LogManager::instance()) {
            LogManager::instance()->setVisible(checked);
        }
    });

    auto sS = [&](const char *t) {
        auto *a = new QAction(t, this);
        a->setCheckable(true);
        screenGroup_->addAction(a);
        return a;
    };

    setScreenSize_ = view_->addMenu("Screen Size");

    actFullScreen_ = sS("Full Screen");
    actMaximize_   = sS("MaxiMize");

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

    //themeToggle_ = new QAction("Toggle Dark/Light", this);
    //sett_->addAction(themeToggle_);

    // add help actions
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
