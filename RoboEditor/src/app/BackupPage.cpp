#include "BackupPage.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ControllerManager.h"
#include "LogManager.h"
#include "ProgressDialog.h"
#include "AppConfig.h"
#include "ui_BackupPage.h"

BackupPage::BackupPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BackupPage),
    totalBackupRequests_(0),
    completedBackupRequests_(0),
    failedBackupRequests_(0)
{
    ui->setupUi(this);
    ui->ConfirmBtn->setEnabled(false);

    selectcontrollerWidget = new selectcontroller(this);
    ui->controllerWidget->setLayout(new QVBoxLayout());
    ui->controllerWidget->layout()->addWidget(selectcontrollerWidget);

    connect(ui->refreshBtn, &QPushButton::clicked, this, [=]() {
        selectcontrollerWidget->getControllerState();
    });
    connect(ui->BrowseBtn, &QPushButton::clicked, this, &BackupPage::importAnySpace);
    connect(ui->ConfirmBtn, &QPushButton::clicked, this, &BackupPage::confirmSelection);
    connect(ui->CancelBtn, &QPushButton::clicked, this, [this]() { this->window()->close(); });

    connect(selectcontrollerWidget, &selectcontroller::controllerSelectionChanged, this, [this]() {
        updateConfirmState();
    });

    // ControllerManager의 백업 완료/실패 시그널을 받도록 연결
    ControllerManager *manager = ControllerManager::instance();
    connect(manager, &ControllerManager::backupCompleted, this, &BackupPage::onBackupCompleted);
    connect(manager, &ControllerManager::backupFailed, this, &BackupPage::onBackupFailed);
}

BackupPage::~BackupPage() = default;

void BackupPage::confirmSelection()
{
    selectedControllerList = selectcontrollerWidget->getSelectedControllers();

    if (selectedControllerList.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select at least one controller."));
        return;
    }

    if (selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a backup destination folder."));
        return;
    }

    // 백업 카운터 초기화
    totalBackupRequests_     = selectedControllerList.size();
    completedBackupRequests_ = 0;
    failedBackupRequests_    = 0;

    qDebug() << "Backup to:" << selectedBackupDir;

    ControllerManager *manager = ControllerManager::instance();

    // 기존 dialog 있으면 정리
    if (backupProgressDialog_) {
        backupProgressDialog_->close();
        backupProgressDialog_->deleteLater();
        backupProgressDialog_ = nullptr;
    }

    // 새 dialog 생성
    backupProgressDialog_ = new ProgressDialog(this);
    backupProgressDialog_->reset();

    connect(backupProgressDialog_, &ProgressDialog::cancelRequested, this, [this]() {
        if (backupProgressDialog_) {
            backupProgressDialog_->close();
        }
    });

    backupProgressDialog_->setWindowTitle("Backup in progress...");
    backupProgressDialog_->setTotalCount(totalBackupRequests_);
    backupProgressDialog_->setCurrentIndex(0);
    backupProgressDialog_->setSerialNumber("-");
    backupProgressDialog_->setProgress(0);
    backupProgressDialog_->show();

    // Disable controls during backup
    if (ui->ConfirmBtn)
        ui->ConfirmBtn->setEnabled(false);

    // 각 선택된 제어기에 대해 SFTP 기반 백업 요청
    int idx = 0;
    for (const QString &serialNumber : selectedControllerList) {
        ++idx;

        ControllerInfo info = manager->getController(serialNumber);

        if (info.serialNumber.isEmpty()) {
            qWarning() << "[BackupPage] Unknown controller:" << serialNumber;
            onBackupFailed(serialNumber, tr("Controller is not registered."));
            continue;
        }

        if (!info.isConnected) {
            qWarning() << "[BackupPage] Controller offline:" << serialNumber;
            onBackupFailed(serialNumber, tr("Controller is offline."));
            continue;
        }

        backupProgressDialog_->setSerialNumber(serialNumber);

        qDebug() << "[BackupPage] Backup request for:" << serialNumber;

        bool ok = manager->backupRequest(serialNumber, selectedBackupDir);

        // "백업 요청을 아예 시작 못했을 때"만 즉시 실패 처리
        if (!ok) {
            onBackupFailed(serialNumber, tr("Failed to start backup request."));
        }
    }
}

void BackupPage::importAnySpace()
{
    QString dirName = QFileDialog::getExistingDirectory(
            this,
            "Select Workspace",
            AppConfig::getBackupPath(),         // 초기 디렉토리 경로
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );

    if (dirName.isEmpty())  // 사용자가 취소한 경우
        return;

    selectedBackupDir = dirName;
    ui->selectedDir->setText(selectedBackupDir);

    updateConfirmState();
}

void BackupPage::updateConfirmState()
{
    bool hasControllers = !selectcontrollerWidget->getSelectedControllers().isEmpty();
    bool hasDirectory   = !selectedBackupDir.isEmpty();
    ui->ConfirmBtn->setEnabled(hasControllers && hasDirectory);
}

// 백업 완료 처리
void BackupPage::onBackupCompleted(const QString &serialNumber)
{
    if (totalBackupRequests_ <= 0)
        return;

    completedBackupRequests_++;
    qDebug() << "[BackupPage] Backup completed:" << serialNumber << "(" << completedBackupRequests_
             << "/" << totalBackupRequests_ << ")";

    QString msg = QString("Backup completed: %1 (%2/%3)")
                          .arg(serialNumber)
                          .arg(completedBackupRequests_)
                          .arg(totalBackupRequests_);

    LogManager::append(msg);

    int doneCount = completedBackupRequests_ + failedBackupRequests_;

    if (backupProgressDialog_ && totalBackupRequests_ > 0) {
        backupProgressDialog_->setCurrentIndex(doneCount);
        int percent = (doneCount * 100) / totalBackupRequests_;
        backupProgressDialog_->setProgress(percent);
    }
    // 모든 백업 완료 시
    if (doneCount >= totalBackupRequests_) {
        // ProgressDialog 닫고 정리
        if (backupProgressDialog_) {
            backupProgressDialog_->setFinishedMode(true);

            if (failedBackupRequests_ == 0) {
                backupProgressDialog_->setStatusText(
                        QString("Backup completed. (Total %1 controller(s), all succeeded)")
                                .arg(completedBackupRequests_));
            } else {
                backupProgressDialog_->setStatusText(
                        QString("Backup completed.\nSucceeded: %1, Failed: %2")
                                .arg(completedBackupRequests_)
                                .arg(failedBackupRequests_));
            }
        }

        // 버튼 다시 활성화
        if (ui->ConfirmBtn)
            ui->ConfirmBtn->setEnabled(true);
    }
}

// 백업 실패 처리
void BackupPage::onBackupFailed(const QString &serialNumber, const QString &error)
{
    if (totalBackupRequests_ <= 0)
        return;

    failedBackupRequests_++;
    qWarning() << "[BackupPage] Backup failed:" << serialNumber << error;

    int doneCount = completedBackupRequests_ + failedBackupRequests_;

    QString msg = QString("Backup failed: %1 (%2/%3)")
                          .arg(serialNumber)
                          .arg(failedBackupRequests_)
                          .arg(totalBackupRequests_);

    LogManager::append(msg);

    // 진행률 퍼센트 갱신
    if (backupProgressDialog_ && totalBackupRequests_ > 0) {
        backupProgressDialog_->setCurrentIndex(doneCount);
        int percent = (doneCount * 100) / totalBackupRequests_;
        backupProgressDialog_->setProgress(percent);
    }

    // 모든 백업 처리 완료 시
    if (doneCount >= totalBackupRequests_) {
        backupProgressDialog_->setFinishedMode(true);

        // ProgressDialog 닫고 정리
        if (backupProgressDialog_) {
            backupProgressDialog_->setStatusText(
                    QString("Backup completed.\nSucceeded: %1, Failed: %2")
                            .arg(completedBackupRequests_)
                            .arg(failedBackupRequests_));
        }

        // 버튼 다시 활성화
        if (ui->ConfirmBtn)
            ui->ConfirmBtn->setEnabled(true);
    }
}
