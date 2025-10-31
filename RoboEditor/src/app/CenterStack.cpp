#include "CenterStack.h"

#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "ApplyPage.h"
#include "BackupPage.h"
#include "ComparePage.h"
#include "ModifyPage.h"
#include "OpenFilePage.h"

CenterStack::CenterStack(QWidget *parent) : QWidget(parent)
{
    stack_ = new QStackedWidget;
    cmp_   = new ComparePage;
    bkp_   = new BackupPage;
    ofp_   = new OpenFilePage;
    alp_   = new ApplyPage;
    mfp_   = new ModifyPage;

    idxC_ = stack_->addWidget(cmp_);
    idxB_ = stack_->addWidget(bkp_);
    idxO_ = stack_->addWidget(ofp_);
    idxA_ = stack_->addWidget(alp_);
    idxM_ = stack_->addWidget(mfp_);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(stack_);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(cmp_, &ComparePage::uiCompareClicked, this, [=](const QString &L, const QString &R) {
        emit compareRequested(L, R);
        openCompareResult(L, R);
    });
    connect(bkp_, &BackupPage::uiBackupClicked, this, &CenterStack::backupRequested);
    connect(ofp_, &OpenFilePage::uiOpenFileClicked, this, &CenterStack::openFileRequested);
    connect(alp_, &ApplyPage::uiApplyClicked, this, &CenterStack::applyRequested);
    connect(mfp_, &ModifyPage::uiModifyClicked, this, &CenterStack::modifyRequested);
}

void CenterStack::openCompareResult(const QString &left, const QString &right)
{
    QWidget *result = new QWidget;
    auto    *ly     = new QVBoxLayout(result);
    ly->addWidget(new QLabel(QString("Result for:\nL=%1\nR=%2").arg(left, right)));
    ly->addStretch();

    int idx = stack_->addWidget(result);
    stack_->setCurrentIndex(idx);
}
void CenterStack::showCompare()
{
    stack_->setCurrentIndex(idxC_);
}
void CenterStack::showBackup()
{
    stack_->setCurrentIndex(idxB_);
}
void CenterStack::showOpenFile()
{
    stack_->setCurrentIndex(idxO_);
}
void CenterStack::showModify()
{
    stack_->setCurrentIndex(idxM_);
}
void CenterStack::showApply()
{
    stack_->setCurrentIndex(idxA_);
}

void CenterStack::showModifyWithCompare()
{
    // ModifyPage로 전환
    stack_->setCurrentIndex(idxM_);

    // ModifyPage의 Compare 기능 활성화
    if (mfp_) {
        mfp_->showCompare();
    }
}
