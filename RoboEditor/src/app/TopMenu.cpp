#include "TopMenu.h"
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>

TopMenu::TopMenu(QMainWindow* mw): QObject(mw), mw_(mw) { build(); }

void TopMenu::build() {
    auto* mb = mw_->menuBar();
    file_ = mb->addMenu("File");
    edit_ = mb->addMenu("Edit");
    view_ = mb->addMenu("View");
    help_ = mb->addMenu("Help");

    // View > Show Log
    showLogMenu_ = view_->addMenu("Show Log");

    actLogVisible_ = new QAction("Visible", this);
    actLogVisible_->setCheckable(true); actLogVisible_->setChecked(true);
    showLogMenu_->addAction(actLogVisible_);
    showLogMenu_->addSeparator();

    posGroup_ = new QActionGroup(this); posGroup_->setExclusive(true);
    auto mk = [&](const char* t){ auto* a=new QAction(t,this); a->setCheckable(true); posGroup_->addAction(a); return a; };
    showLogMenu_->addAction(mk("Log → Bottom"));
    showLogMenu_->addAction(mk("Log → Right"));
    showLogMenu_->addAction(mk("Log → Left"));
    showLogMenu_->addAction(mk("Log → Top"));
    showLogMenu_->addAction(mk("Log → Float"));
    showLogMenu_->addAction(mk("Log → Center Bottom"));
    posGroup_->actions().front()->setChecked(true);

    actToggle_ = new QAction("Toggle Log", this);
    actToggle_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    view_->addAction(actToggle_);

    connect(actToggle_, &QAction::triggered, actLogVisible_, &QAction::toggle);
}
