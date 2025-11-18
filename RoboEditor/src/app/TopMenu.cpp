#include "TopMenu.h"

#include <QAction>
#include <QActionGroup>
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

    // View > Show Log
    showLogMenu_ = view_->addMenu("Show Log");

    actToggleLog_ = new QAction("Show Log Viewer", this);
    actToggleLog_->setCheckable(true);
    actToggleLog_->setChecked(true);
    actToggleLog_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));

    showLogMenu_->addAction(actToggleLog_);
    showLogMenu_->addSeparator();

    posGroup_ = new QActionGroup(this);
    posGroup_->setExclusive(true);
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

    themeToggle_ = new QAction("Toggle Dark/Light", this);

    actChangePswd_ = new QAction("Change Password", this);

    //sett_->addAction(actChangePswd_);
    //sett_->addAction(themeToggle_);

    connect(actToggleLog_, &QAction::toggled, this, [this](bool checked) {
        if (LogManager::instance()) {
            LogManager::instance()->setVisible(checked);
        }
    });
    connect(actChangePswd_, &QAction::triggered, this, &TopMenu::onChangePasswordTriggered);
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
