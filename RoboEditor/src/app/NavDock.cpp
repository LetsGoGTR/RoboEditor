// NavDock.cpp
#include "NavDock.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

NavDock::NavDock(QMainWindow *mw) : QObject(mw), mw_(mw)
{
    build();
}

void NavDock::build()
{
    auto *left = new QWidget;
    auto *v    = new QVBoxLayout(left);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(8);
    auto mk = [&](const QString &t) {
        auto *b = new QPushButton(t);
        b->setMinimumWidth(180);
        return b;
    };
    auto *bC = mk("비교하기"), *bB = mk("백업하기"), *bA = mk("적용하기"), *bM = mk("수정하기"),
         *bL = mk("확인하기");
    v->addWidget(bC);
    v->addWidget(bB);
    v->addWidget(bA);
    v->addWidget(bM);
    v->addWidget(bL);
    v->addStretch();

    dock_ = new QDockWidget("Navigation", mw_);
    dock_->setWidget(left);
    dock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    mw_->addDockWidget(Qt::LeftDockWidgetArea, dock_);

    connect(bC, &QPushButton::clicked, this, &NavDock::clickCompare);
    connect(bB, &QPushButton::clicked, this, &NavDock::clickBackup);
    connect(bA, &QPushButton::clicked, this, &NavDock::clickApply);
    connect(bM, &QPushButton::clicked, this, &NavDock::clickModify);
    connect(bL, &QPushButton::clicked, this, &NavDock::clickOpenFile);
}
