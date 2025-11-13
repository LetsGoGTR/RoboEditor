#include "BackupPage.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ControllerManager.h"
#include "ui_BackupPage.h"
#include "ProgressDialog.h"
#include "ControllerManager.h"

BackupPage::BackupPage(QWidget *parent) : QWidget(parent), ui(new Ui::BackupPage), totalBackupRequests_(0), completedBackupRequests_(0), failedBackupRequests_(0)
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
    connect(manager, &ControllerManager::backupCompleted,
            this, &BackupPage::onBackupCompleted);
    connect(manager, &ControllerManager::backupFailed,
            this, &BackupPage::onBackupFailed);

}

BackupPage::~BackupPage() = default;

void BackupPage::confirmSelection()
{
    selectedControllerList = selectcontrollerWidget->getSelectedControllers();

    if (selectedControllerList.isEmpty()) {
        QMessageBox::warning(this, "오류", "제어기를 선택해주세요.");
        return;
    }

    if (selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, "오류", "백업 저장 경로를 선택해주세요.");
        return;
    }

    // 백업 카운터 초기화
    totalBackupRequests_ = selectedControllerList.size();
    completedBackupRequests_ = 0;
    failedBackupRequests_ = 0;

    qDebug() << "Backup to:" << selectedBackupDir;

    ControllerManager *manager = ControllerManager::instance();

    if (!backupProgressDialog_) {
        backupProgressDialog_ = new ProgressDialog(this);
        connect(backupProgressDialog_, &ProgressDialog::cancelRequested,
                this, [this]() {
                    // 일단 취소 누르면 창만 닫는 정도로
                    if (backupProgressDialog_) {
                        backupProgressDialog_->close();
                    }
                    // 나중에 진짜 "백업 중단" 로직 붙이고 싶으면 여기에서 처리
                });
    }
    backupProgressDialog_->setWindowTitle("백업 진행 중...");
    backupProgressDialog_->setTotalCount(totalBackupRequests_);
    backupProgressDialog_->setCurrentIndex(0);
    backupProgressDialog_->setSerialNumber("-");
    backupProgressDialog_->setProgress(0);
    backupProgressDialog_->show();

    // Disable controls during backup
    if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(false);

    // 각 선택된 제어기에 대해 SFTP 기반 백업 요청
    int idx = 0;
    for (const QString &serialNumber : selectedControllerList) {
        ++idx;

        ControllerInfo info = manager->getController(serialNumber);

        if (info.serialNumber.isEmpty()) {
            qWarning() << "[BackupPage] Unknown controller:" << serialNumber;
            onBackupFailed(serialNumber, tr("등록 정보가 없는 제어기입니다."));
            continue;
        }

        if (!info.isConnected) {
            qWarning() << "[BackupPage] Controller offline:" << serialNumber;
            onBackupFailed(serialNumber, tr("제어기가 오프라인 상태입니다."));
            continue;
        }

        if (backupProgressDialog_) {
            backupProgressDialog_->setCurrentIndex(idx);
            backupProgressDialog_->setSerialNumber(serialNumber);
        }

        qDebug() << "[BackupPage] Backup request for:" << serialNumber;

        bool ok = manager->backupRequest(serialNumber, selectedBackupDir);

        // "백업 요청을 아예 시작 못했을 때"만 즉시 실패 처리
        if (!ok) {
            onBackupFailed(serialNumber, tr("백업 요청을 시작하지 못했습니다."));
        }
    }


}

void BackupPage::importAnySpace()
{
    QString dirName = QFileDialog::getExistingDirectory(
            this,                // 부모 위젯 포인터 (nullptr 대신 this 권장)
            "Select Workspace",  // 대화상자 제목
            "C:/backup",         // 초기 디렉토리 경로
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks  // 폴더만 표시
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
    completedBackupRequests_++;
    qDebug() << "[BackupPage] Backup completed:" << serialNumber
             << "(" << completedBackupRequests_ << "/" << totalBackupRequests_ << ")";

    int doneCount = completedBackupRequests_ + failedBackupRequests_;

    if (backupProgressDialog_ && totalBackupRequests_ > 0) {
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
                        QString("백업이 완료되었습니다. (총 %1대, 모두 성공)")
                                .arg(completedBackupRequests_));
            } else {
                backupProgressDialog_->setStatusText(
                        QString("백업이 완료되었습니다.\n성공: %1대, 실패: %2대")
                                .arg(completedBackupRequests_)
                                .arg(failedBackupRequests_));
            }
        }

        // 버튼 다시 활성화
        if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(true);
    }
}

// 백업 실패 처리
void BackupPage::onBackupFailed(const QString &serialNumber, const QString &error)
{
    failedBackupRequests_++;
    qWarning() << "[BackupPage] Backup failed:" << serialNumber << error;

    int doneCount = completedBackupRequests_ + failedBackupRequests_;

    // 진행률 퍼센트 갱신
    if (backupProgressDialog_ && totalBackupRequests_ > 0) {
        int percent = (doneCount * 100) / totalBackupRequests_;
        backupProgressDialog_->setProgress(percent);
    }

    // 모든 백업 처리 완료 시
    if (doneCount >= totalBackupRequests_) {

        backupProgressDialog_->setFinishedMode(true);

        // ProgressDialog 닫고 정리
        if (backupProgressDialog_) {
            backupProgressDialog_->setStatusText(
                    QString("백업이 완료되었습니다.\n성공: %1대, 실패: %2대")
                            .arg(completedBackupRequests_)
                            .arg(failedBackupRequests_));

        }

        // 버튼 다시 활성화
        if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(true);
    }
}
