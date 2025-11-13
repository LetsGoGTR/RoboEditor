#include "ProgressDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("진행 중...");
    resize(400, 180);

    serialLabel_ = new QLabel("현재 제어기: -");
    countLabel_  = new QLabel("0 / 0");
    timeLabel_   = new QLabel("0s...");
    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);

    cancelBtn_ = new QPushButton("취소");

    auto layout = new QVBoxLayout(this);
    layout->addWidget(serialLabel_);
    layout->addWidget(countLabel_);
    layout->addWidget(timeLabel_);
    layout->addWidget(progressBar_);
    layout->addWidget(cancelBtn_);

    connect(cancelBtn_, &QPushButton::clicked,
            this, &ProgressDialog::cancelRequested);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout,
            this, &ProgressDialog::updateElapsedTime);
    timer_->start(1000);
    elapsed_.start();
}

void ProgressDialog::setTotalCount(int total)
{
    totalCount_ = total;
    countLabel_->setText(QString("%1 / %2").arg(currentIndex_).arg(totalCount_));
}

void ProgressDialog::setCurrentIndex(int index)
{
    currentIndex_ = index;
    countLabel_->setText(QString("%1 / %2").arg(currentIndex_).arg(totalCount_));
}

void ProgressDialog::setSerialNumber(const QString &sn)
{
    serialLabel_->setText(QString("현재 제어기: %1").arg(sn));
}

void ProgressDialog::setProgress(int percent)
{
    progressBar_->setValue(percent);
}

void ProgressDialog::updateElapsedTime()
{
    int secs = elapsed_.elapsed() / 1000;
    timeLabel_->setText(QString("%1s...").arg(secs));
}
