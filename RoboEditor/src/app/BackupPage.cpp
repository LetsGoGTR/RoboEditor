#include "BackupPage.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ControllerManager.h"
#include "ui_BackupPage.h"

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

    QMessageBox::information(this, "백업 시작",
                             QString("선택된 %1개 제어기의 백업이 시작되었습니다.\n저장 경로: %2")
                                     .arg(selectedControllerList.size())
                                     .arg(selectedBackupDir));

    ControllerManager *manager = ControllerManager::instance();

    // Disable controls during backup
    if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(false);
    if (ui->BrowseBtn) ui->BrowseBtn->setEnabled(false);
    if (ui->refreshBtn) ui->refreshBtn->setEnabled(false);

    // 각 선택된 제어기에 대해 SFTP 기반 백업 요청
    for (const QString &serialNumber : selectedControllerList) {
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

        qDebug() << "[BackupPage] Backup request for:" << serialNumber;

        bool ok = manager->backupRequest(serialNumber, selectedBackupDir);
        if (ok) {
            onBackupCompleted(serialNumber);
        } else {
            onBackupFailed(serialNumber, tr("SFTP 백업 실패"));
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

    // 모든 백업 완료 시 창 닫기
    if (completedBackupRequests_ + failedBackupRequests_ >= totalBackupRequests_) {
        QString message;
        if (failedBackupRequests_ > 0) {
            message = QString("백업 완료!\n성공: %1개, 실패: %2개")
                              .arg(completedBackupRequests_)
                              .arg(failedBackupRequests_);
        } else {
            message = QString("모든 백업이 성공적으로 완료되었습니다!\n완료: %1개")
                              .arg(completedBackupRequests_);
        }

        QMessageBox::information(this, "백업 완료", message);
        // 창을 자동으로 닫지 않고 유지하여 후속 작업이 가능하도록 한다.
        // Re-enable controls after processing
        if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(true);
        if (ui->BrowseBtn) ui->BrowseBtn->setEnabled(true);
        if (ui->refreshBtn) ui->refreshBtn->setEnabled(true);
        // Re-enable controls after processing
        if (ui->ConfirmBtn) ui->ConfirmBtn->setEnabled(true);
        if (ui->BrowseBtn) ui->BrowseBtn->setEnabled(true);
        if (ui->refreshBtn) ui->refreshBtn->setEnabled(true);
        // this->window()->close();
    }
}

// 백업 실패 처리
void BackupPage::onBackupFailed(const QString &serialNumber, const QString &error)
{
    failedBackupRequests_++;
    qWarning() << "[BackupPage] Backup failed:" << serialNumber << error;

    // 모든 백업 처리 완료 시 창 닫기
    if (completedBackupRequests_ + failedBackupRequests_ >= totalBackupRequests_) {
        QString message = QString("백업 완료!\n성공: %1개, 실패: %2개")
                                  .arg(completedBackupRequests_)
                                  .arg(failedBackupRequests_);

        QMessageBox::warning(this, "백업 완료", message);
        // 창을 자동으로 닫지 않는다.
        // this->window()->close();
    }
}
