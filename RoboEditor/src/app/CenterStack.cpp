#include "CenterStack.h"
#include "ComparePage.h"
#include "BackupPage.h"

#include <QMainWindow>
#include <QStackedWidget>

CenterStack::CenterStack(QMainWindow* mw)
    : QObject(mw)
{
    stack_ = new QStackedWidget(mw);

    cmp_ = new ComparePage;
    bkp_ = new BackupPage;

    idxC_ = stack_->addWidget(cmp_);
    idxB_ = stack_->addWidget(bkp_);

    mw->setCentralWidget(stack_);

    // ComparePage -> CenterStack relay
    QObject::connect(cmp_, &ComparePage::requestCompare,
                     this, &CenterStack::compareRequested);

    // BackupPage -> CenterStack relay
    QObject::connect(bkp_, &BackupPage::requestBackup,
                     this, &CenterStack::backupRequested);
}

void CenterStack::showCompare() {
    stack_->setCurrentIndex(idxC_);
}

void CenterStack::showBackup() {
    stack_->setCurrentIndex(idxB_);
}
