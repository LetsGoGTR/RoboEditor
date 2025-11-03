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
    ofp_   = new OpenFilePage;
    mfp_   = new ModifyPage;

    idxC_ = stack_->addWidget(cmp_);
    idxO_ = stack_->addWidget(ofp_);
    idxM_ = stack_->addWidget(mfp_);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(stack_);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(cmp_, &ComparePage::uiCompareClicked, this, [=](const QString &L, const QString &R) {
        emit compareRequested(L, R);
        openCompareResult(L, R);
    });
    connect(ofp_, &OpenFilePage::uiOpenFileClicked, this, &CenterStack::openFileRequested);
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

void CenterStack::showOpenFile()
{
    stack_->setCurrentIndex(idxO_);
}

void CenterStack::showModify()
{
    stack_->setCurrentIndex(idxM_);
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
