#include "ApplyPage.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>

#include "ConfirmSelection.h"
#include "ControllerManager.h"
#include "LogManager.h"
#include "PasswordManager.h"
#include "ProgressDialog.h"
#include "AppConfig.h"
#include "ui_ApplyPage.h"
#include "ui_ConfirmSelection.h"


// 경로를 'backup/...' 형태로 줄여주는 헬퍼 함수
static QString formatDisplayPath(const QString &fullPath)
{
    if (fullPath.isEmpty()) return "";

    QString backupRoot = QDir::cleanPath(AppConfig::getBackupPath());
    QString targetPath = QDir::cleanPath(fullPath);

    // 선택된 경로가 backup 루트 경로로 시작하는지 확인
    if (targetPath.startsWith(backupRoot, Qt::CaseInsensitive)) {
        // 루트 경로만큼 잘라냄 (예: "C:/.../backup" 제거)
        QString relative = targetPath.mid(backupRoot.length());

        // 맨 앞의 슬래시 제거
        if (relative.startsWith('/')) relative.remove(0, 1);

        // "backup/"을 앞에 붙여서 리턴
        return "backup/" + relative;
    }

    // backup 폴더 밖의 경로라면 원래 경로 그대로 표시
    return fullPath;
}

ApplyPage::ApplyPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyPage),
    totalApplyRequests_(0),
    completedApplyRequests_(0),
    failedApplyRequests_(0)
{
    ui->setupUi(this);
    selectbackupWidget     = new selectTableWidget(this);
    selectcontrollerWidget = new selectcontroller(this);

    ui->leftWidget->setLayout(new QVBoxLayout());
    ui->leftWidget->layout()->addWidget(selectbackupWidget);
    ui->rightWidget->setLayout(new QVBoxLayout());
    ui->rightWidget->layout()->addWidget(selectcontrollerWidget);

    QObject::connect(ui->refreshBtn, &QPushButton::clicked, this, &ApplyPage::refreshList);
    QObject::connect(ui->applyBtn, &QPushButton::clicked, this, &ApplyPage::confirmSelection);
    QObject::connect(ui->importBtn, &QPushButton::clicked, this, &ApplyPage::importAnyspace);

    connect(selectbackupWidget,
            &selectTableWidget::folderSelected,
            this,
            [this](const QString &folder) {
                selectedBackupDir = folder;
                ui->selectedDir->setText(formatDisplayPath(selectedBackupDir));
                qDebug() << "ApplyPage received:" << selectedBackupDir;
            });

    ControllerManager *manager = ControllerManager::instance();
    connect(manager, &ControllerManager::applyCompleted, this, &ApplyPage::onApplyCompleted);
    connect(manager, &ControllerManager::applyFailed, this, &ApplyPage::onApplyFailed);
}

//refresh : 제어기 목록 새로고침
void ApplyPage::refreshList()
{
    selectcontrollerWidget->getControllerState();
}

//apply : 선택한 백업 스페이스 정보, 제어기 정보를 서버에 전송
void ApplyPage::confirmSelection()
{
    selectedControllerList = selectcontrollerWidget->getSelectedControllers();

    if (selectedControllerList.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select at least one controller."));
        return;
    }

    if (selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a backup file."));
        return;
    }

    for (const auto &cur : selectedControllerList) {
        qDebug() << "selected:" << cur;
    }

    showPasswordUI();
}

//import : 다른 백업 스페이스 폴더 찾기
void ApplyPage::importAnyspace()
{
    QString fileName =
            QFileDialog::getOpenFileName(nullptr,            // 부모 위젯 포인터
                                         "Select WorkSpace", // 대화 상자 제목
                                         QDir::homePath(),   // 초기 디렉터리 경로
                                         "All Files (*);;YAML Files (*.yaml *.yml);;Python Files "
                                         "(*.py *.srl);;ZIP Archives (*.zip)");

    selectedBackupDir = fileName;
    selectbackupWidget->clearRadioSelection();
    ui->selectedDir->setText(selectedBackupDir);
}

void ApplyPage::showPasswordUI()
{
    PasswordManager manager(this);

    if (manager.exec() == QDialog::Accepted) {
        QString inputPassword = manager.getMasterPassword();
        if (manager.isCorrect(inputPassword)) {
            // 비밀번호 검증 성공
            ConfirmSelection confirmDialog(this);
            confirmDialog.setApplyPage(this);
            apiPassword_ = inputPassword;

            connect(&confirmDialog,
                    &ConfirmSelection::applyRequested,
                    this,
                    [this]() {
                        // 비밀번호 + 최종 확인 후, 여기서부터 실제 처리 시작
                        // 1단계: 먼저 대상 제어기들의 현재 상태를 백업
                        startPreBackup();
                    },
                    Qt::SingleShotConnection);

            if (confirmDialog.exec() == QDialog::Accepted) {
                // 실제 처리(startPreBackup)는 applyRequested 시그널에서 시작됨
            }
        } else {
            LogManager::append("Apply password not match");
            QMessageBox::warning(this, tr("Error"), tr("The password you entered is incorrect."));
        }
    } else {
        LogManager::append("Password dialog cancelled");
    }
}

QString ApplyPage::determineBackupRoot() const
{
    if (selectedBackupDir.isEmpty()) {
        return AppConfig::getBackupPath();
    }

    QDir dir(selectedBackupDir);

    // 선택된 경로가 이미 backup 루트라면 그대로 사용
    if (dir.dirName().compare("backup", Qt::CaseInsensitive) == 0) {
        return dir.path();
    }

    // 일반 구조: C:/backup/<serial>/<serial>_timestamp
    QDir tmp = dir;
    bool ok1 = tmp.cdUp(); // → <serial>
    bool ok2 = tmp.cdUp(); // → backup

    if (ok1 && ok2)
        return tmp.path(); // ex) C:/backup

    // 실패 시 그냥 현재 경로 반환
    return dir.absolutePath();
}

// 사전 백업 시작
void ApplyPage::startPreBackup()
{
    if (selectedControllerList.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No controllers have been selected."));
        return;
    }

    if (preBackupInProgress_) {
        qWarning() << "[ApplyPage] Pre-backup already in progress.";
        return;
    }

    // 진행 다이얼로그 새로 생성 (백업+적용 통합)
    if (applyProgressDialog_) {
        applyProgressDialog_->close();
        applyProgressDialog_->deleteLater();
        applyProgressDialog_ = nullptr;
    }

    applyProgressDialog_ = new ProgressDialog(this);
    applyProgressDialog_->setWindowTitle("Pre-backup and apply in progress...");
    applyProgressDialog_->setSerialNumber("-");
    applyProgressDialog_->setTotalCount(0);   // 상단 countLabel은 전체 개수 대신 따로 쓰므로 0으로 시작
    applyProgressDialog_->setCurrentIndex(0);
    applyProgressDialog_->setProgress(0);
    applyProgressDialog_->setFinishedMode(false);
    applyProgressDialog_->show();

    QApplication::processEvents();

    connect(applyProgressDialog_, &ProgressDialog::cancelRequested, this, [this]() {
        if (applyProgressDialog_) {
            applyProgressDialog_->close();
            // 실제 백업/적용 취소까지 하려면 별도 설계 필요
        }
    });

    ControllerManager *manager = ControllerManager::instance();

    // 상태 검증 (Apply 때와 동일)
    QStringList disconnectedControllers;
    QStringList unknownControllers;
    QStringList runningControllers;

    for (const QString &sn : selectedControllerList) {
        ControllerInfo info = manager->getController(sn);
        if (info.serialNumber.isEmpty()) {
            unknownControllers.append(sn);
            continue;
        }
        if (!info.isConnected) {
            disconnectedControllers.append(sn);
            continue;
        }
        if (info.isRunning) {
            runningControllers.append(sn);
            continue;
        }
    }

    if (!unknownControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("The following controllers are not registered:\n%1")
                        .arg(unknownControllers.join(", ")));
        return;
    }
    if (!disconnectedControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("The following controllers are offline:\n%1")
                        .arg(disconnectedControllers.join(", ")));
        return;
    }
    if (!runningControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("The following controllers are currently running. Apply cannot proceed:\n%1")
                        .arg(runningControllers.join(", ")));
        return;
    }

    // 사전 백업 상태 초기화
    preBackupInProgress_         = true;
    preBackupTotal_              = selectedControllerList.size();
    preBackupCompleted_          = 0;
    preBackupFailed_             = 0;
    preBackupSuccessControllers_.clear();
    preBackupFailedControllers_.clear();

    // Apply 관련 카운터도 초기화
    applyTargetControllers_.clear();
    applyQueue_.clear();
    totalApplyRequests_     = 0;
    completedApplyRequests_ = 0;
    failedApplyRequests_    = 0;
    applyInProgress_        = false;

    // 텍스트 초기 상태
    updateApplyProgressStatus();

    QString backupRoot = determineBackupRoot();
    qDebug() << "[ApplyPage] Pre-backup root:" << backupRoot;

    // 시그널 연결
    connect(manager,
            &ControllerManager::backupCompleted,
            this,
            &ApplyPage::onPreBackupCompleted,
            Qt::UniqueConnection);
    connect(manager,
            &ControllerManager::backupFailed,
            this,
            &ApplyPage::onPreBackupFailed,
            Qt::UniqueConnection);

    // 각 제어기에 대해 백업 요청
    for (const QString &sn : selectedControllerList) {
        qDebug() << "[ApplyPage] Pre-backup request for:" << sn;
        bool ok = manager->backupRequest(sn, backupRoot);
        if (!ok) {
            preBackupFailed_++;
            preBackupFailedControllers_.append(sn);
            LogManager::append(
                    QString("Pre-backup start failed: %1").arg(sn));

            if (applyProgressDialog_ && preBackupTotal_ > 0) {
                int doneCount = preBackupCompleted_ + preBackupFailed_;
                int percent   = (doneCount * 50) / preBackupTotal_; // 0~50
                applyProgressDialog_->setProgress(percent);
            }
            updateApplyProgressStatus();
        }
    }

    int started = preBackupTotal_ - preBackupFailed_;
    if (started == 0) {
        preBackupInProgress_ = false;

        ControllerManager *m = ControllerManager::instance();
        disconnect(m, &ControllerManager::backupCompleted, this, &ApplyPage::onPreBackupCompleted);
        disconnect(m, &ControllerManager::backupFailed, this, &ApplyPage::onPreBackupFailed);

        QMessageBox::warning(
                this,
                tr("Backup Error"),
                tr("Failed to start the pre-backup request.\nThe apply operation will be canceled."));
    }
}

// 사전 백업 콜백
void ApplyPage::onPreBackupCompleted(const QString &serialNumber)
{
    if (!preBackupInProgress_)
        return;

    preBackupCompleted_++;
    preBackupSuccessControllers_.append(serialNumber);

    int doneCount = preBackupCompleted_ + preBackupFailed_;

    QString msg =
            QString("Pre-backup completed: %1 (done: %2/%3, success: %4, fail: %5)")
                    .arg(serialNumber)
                    .arg(doneCount)
                    .arg(preBackupTotal_)
                    .arg(preBackupCompleted_)
                    .arg(preBackupFailed_);
    LogManager::append(msg);

    // 백업 진행률: 0~50% 사이에서만 사용
    if (applyProgressDialog_ && preBackupTotal_ > 0) {
        int doneCount = preBackupCompleted_ + preBackupFailed_;
        int percent   = (doneCount * 50) / preBackupTotal_; // 0~50
        applyProgressDialog_->setProgress(percent);
    }

    updateApplyProgressStatus();

    if (doneCount < preBackupTotal_)
        return;

    // 모든 사전 백업 응답 수신 완료
    preBackupInProgress_ = false;

    ControllerManager *manager = ControllerManager::instance();
    disconnect(manager, &ControllerManager::backupCompleted, this, &ApplyPage::onPreBackupCompleted);
    disconnect(manager, &ControllerManager::backupFailed, this, &ApplyPage::onPreBackupFailed);

    // 성공한 제어기가 하나도 없으면 → Apply 자체 불가
    if (preBackupSuccessControllers_.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Backup Error"),
                tr("Pre-backup failed for all controllers.\nApply cannot be performed."));
        return;
    }

    // 일부만 성공한 경우:
    applyTargetControllers_ = preBackupSuccessControllers_;

    if (!preBackupFailedControllers_.isEmpty()) {
        QString failList = preBackupFailedControllers_.join(", ");
        QString warnMsg  = QString(
                                  "Pre-backup failed for some controllers.\n"
                                  "The following controllers will be excluded from apply:\n%1")
                                  .arg(failList);
        QMessageBox::warning(this, tr("Pre-backup Warning"), warnMsg);

        LogManager::append(
                QString("Pre-backup failed controllers (skipped in apply): %1")
                        .arg(failList));
    }

    // 이제 실제 Apply 큐 시작
    startApplyQueue();
}

void ApplyPage::onPreBackupFailed(const QString &serialNumber, const QString &error)
{
    if (!preBackupInProgress_)
        return;

    preBackupFailed_++;
    preBackupFailedControllers_.append(serialNumber);

    int doneCount = preBackupCompleted_ + preBackupFailed_;

    QString msg =
            QString("Pre-backup failed: %1 (done: %2/%3, success: %4, fail: %5) - %6")
                    .arg(serialNumber)
                    .arg(doneCount)
                    .arg(preBackupTotal_)
                    .arg(preBackupCompleted_)
                    .arg(preBackupFailed_)
                    .arg(error);
    LogManager::append(msg);

    // 백업 진행률: 0~50% 사이에서만 사용
    if (applyProgressDialog_ && preBackupTotal_ > 0) {
        int doneCount = preBackupCompleted_ + preBackupFailed_;
        int percent   = (doneCount * 50) / preBackupTotal_;
        applyProgressDialog_->setProgress(percent);
    }

    updateApplyProgressStatus();

    if (doneCount < preBackupTotal_)
        return;

    // 전체 사전 백업 응답 완료
    preBackupInProgress_ = false;

    ControllerManager *manager = ControllerManager::instance();
    disconnect(manager, &ControllerManager::backupCompleted, this, &ApplyPage::onPreBackupCompleted);
    disconnect(manager, &ControllerManager::backupFailed, this, &ApplyPage::onPreBackupFailed);

    if (preBackupSuccessControllers_.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Backup Failed"),
                tr("All pre-backup operations failed.\nApply cannot be performed."));
        return;
    }

    applyTargetControllers_ = preBackupSuccessControllers_;

    if (!preBackupFailedControllers_.isEmpty()) {
        QString failList = preBackupFailedControllers_.join(", ");
        QString warnMsg  = QString(
                                  "Pre-backup failed for some controllers.\n"
                                  "The following controllers will be excluded from apply:\n%1")
                                  .arg(failList);
        QMessageBox::warning(this, tr("Pre-backup Warning"), warnMsg);

        LogManager::append(
                QString("Pre-backup failed controllers (skipped in apply): %1")
                        .arg(failList));
    }

    startApplyQueue();
}

// Apply 큐 시작
void ApplyPage::startApplyQueue()
{
    if (applyTargetControllers_.isEmpty() || selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No controller or backup file information is available."));
        return;
    }

    ControllerManager *manager = ControllerManager::instance();
    QStringList        disconnectedControllers;
    QStringList        unknownControllers;
    QStringList        runningControllers;

    manager->pauseStateUpdates();

    for (const QString &sn : applyTargetControllers_) {
        ControllerInfo info = manager->getController(sn);
        if (info.serialNumber.isEmpty()) {
            unknownControllers.append(sn);
            continue;
        }

        if (!info.isConnected) {
            disconnectedControllers.append(sn);
            continue;
        }

        if (info.isRunning) {
            runningControllers.append(sn);
            continue;
        }
    }

    if (!unknownControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("The following controllers are not registered:\n%1")
                        .arg(unknownControllers.join(", ")));

        for (const QString &sn : unknownControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is NOT REGISTERED").arg(sn);
            LogManager::append(msg);
        }

        {
            QString msg = QString("Apply aborted: Unknown controller(s) selected (%1)")
            .arg(unknownControllers.join(", "));
            LogManager::append(msg);
        }

        return;
    }

    if (!disconnectedControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("The following controllers are offline:\n%1")
                        .arg(disconnectedControllers.join(", ")));

        for (const QString &sn : disconnectedControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is OFFLINE").arg(sn);
            LogManager::append(msg);
        }

        {
            QString msg = QString("Apply aborted: Offline controller(s) detected (%1)")
            .arg(disconnectedControllers.join(", "));
            LogManager::append(msg);
        }

        return;
    }

    if (!runningControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                tr("Error"),
                QString("Some controllers are currently running. The apply operation has been canceled:\n%1")
                        .arg(runningControllers.join(", ")));

        for (const auto &sn : runningControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is RUNNING").arg(sn);
            LogManager::append(msg);
        }

        {
            QString msg = QString("Apply aborted: %1 controller(s) running (%2)")
            .arg(runningControllers.size())
                    .arg(runningControllers.join(", "));
            LogManager::append(msg);
        }
        return;
    }

    // 큐 및 카운터 초기화 (Apply 단계용)
    applyQueue_.clear();
    totalApplyRequests_     = applyTargetControllers_.size();
    completedApplyRequests_ = 0;
    failedApplyRequests_    = 0;
    applyInProgress_        = true;

    // 진행 다이얼로그는 재사용, 게이지는 "백업 단계에서 올라간 값" 유지
    if (applyProgressDialog_) {
        applyProgressDialog_->setWindowTitle("Apply in progress...");
        applyProgressDialog_->setTotalCount(totalApplyRequests_);
        applyProgressDialog_->setCurrentIndex(0);
        applyProgressDialog_->setFinishedMode(false);
        applyProgressDialog_->show();
    }

    if (ui->applyBtn)
        ui->applyBtn->setEnabled(false);

    // Apply 큐 구성
    for (const QString &sn : applyTargetControllers_) {
        applyQueue_.enqueue(sn);
    }

    // Apply 시작 시점 텍스트 갱신
    updateApplyProgressStatus();

    QTimer::singleShot(0, this, &ApplyPage::processNextApply);
}

// 큐 처리
void ApplyPage::processNextApply()
{
    if (applyQueue_.isEmpty()) {
        qDebug() << "[ApplyPage] Apply queue finished.";
        onAllAppliesCompleted();
        return;
    }

    QString serialNumber = applyQueue_.dequeue();

    qDebug() << "[ApplyPage] Applying to:" << serialNumber << "backup dir:" << selectedBackupDir;

    if (applyProgressDialog_) {
        applyProgressDialog_->setSerialNumber(serialNumber);
    }

    bool ok = ControllerManager::instance()->applyRequest(
            serialNumber, selectedBackupDir, apiPassword_);
    if (!ok) {
        QString msg = QString("[%1] Apply request could not be started").arg(serialNumber);
        LogManager::append(msg);
        onApplyFailed(serialNumber, tr("Failed to start apply request."));
    }
}

// 개별 적용 성공
void ApplyPage::onApplyCompleted(const QString &serialNumber)
{
    if (!applyInProgress_)
        return;

    completedApplyRequests_++;
    QString msg =
            QString("Apply completed: %1 (%2/%3)")
                    .arg(serialNumber)
                    .arg(completedApplyRequests_)
                    .arg(totalApplyRequests_);
    LogManager::append(msg);

    int doneCount = completedApplyRequests_ + failedApplyRequests_;

    if (applyProgressDialog_ && totalApplyRequests_ > 0) {
        applyProgressDialog_->setCurrentIndex(doneCount);

        // Apply 진행률: 백업에서 0~50, Apply는 50~100 구간 사용
        int percent = 50 + (doneCount * 50) / totalApplyRequests_;
        if (percent > 99) percent = 99; // 전체 완료 전까지는 99까지만
        applyProgressDialog_->setProgress(percent);
    }

    updateApplyProgressStatus();

    processNextApply();
}

// 개별 적용 실패
void ApplyPage::onApplyFailed(const QString &serialNumber, const QString &error)
{
    if (!applyInProgress_)
        return;

    failedApplyRequests_++;
    qWarning() << "[ApplyPage] Apply failed:" << serialNumber << error;
    QString msg =
            QString("[Apply failed: %1 (%2/%3) - %4]")
                    .arg(serialNumber)
                    .arg(failedApplyRequests_)
                    .arg(totalApplyRequests_)
                    .arg(error);
    LogManager::append(msg);

    int doneCount = completedApplyRequests_ + failedApplyRequests_;

    if (applyProgressDialog_ && totalApplyRequests_ > 0) {
        applyProgressDialog_->setCurrentIndex(doneCount);

        int percent = 50 + (doneCount * 50) / totalApplyRequests_;
        if (percent > 99) percent = 99;
        applyProgressDialog_->setProgress(percent);
    }

    updateApplyProgressStatus();

    processNextApply();
}

// 모든 Apply 완료 시
void ApplyPage::onAllAppliesCompleted()
{
    qDebug() << "[ApplyPage] All applies processed.";
    LogManager::append("All applies processed");
    applyInProgress_ = false;
    ControllerManager::instance()->resumeStateUpdates();

    if (applyProgressDialog_) {
        applyProgressDialog_->setProgress(100);
        applyProgressDialog_->setFinishedMode(true);

        QString statusText;
        if (failedApplyRequests_ > 0) {
            statusText = QString("Apply completed.\nSucceeded: %1, Failed: %2")
                                 .arg(completedApplyRequests_)
                                 .arg(failedApplyRequests_);
        } else {
            statusText = QString("Apply completed. (Total %1 controller(s), all succeeded)")
                                 .arg(completedApplyRequests_);
        }
        applyProgressDialog_->setStatusText(statusText);
    }

    if (ui->applyBtn)
        ui->applyBtn->setEnabled(true);
}

void ApplyPage::updateApplyProgressStatus()
{
    if (!applyProgressDialog_)
        return;

    // 사전 백업 진행 상황
    int      backupDone = preBackupCompleted_ + preBackupFailed_;
    QString  backupLine = QString("Pre-backup: %1 / %2")
                                 .arg(backupDone)
                                 .arg(preBackupTotal_);

    // 적용 진행 상황
    QString applyLine;
    if (totalApplyRequests_ > 0) {
        int applyDone = completedApplyRequests_ + failedApplyRequests_;
        applyLine     = QString("Apply: %1 / %2")
                            .arg(applyDone)
                            .arg(totalApplyRequests_);
    } else {
        applyLine = QString("Apply: 0 / 0");
    }

    // 위쪽 "0 / 0" 라벨(countLabel_)에 표시
    applyProgressDialog_->setCountText(backupLine + "\n" + applyLine);

    // 아래쪽 statusLabel_은 비워두거나, 다른 용도로 쓸 경우만 세팅
    applyProgressDialog_->setStatusText(QString());
}

ApplyPage::~ApplyPage()
{
    delete ui;
}
