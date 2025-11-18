#include "ApplyPage.h"

#include <QTimer>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ConfirmSelection.h"
#include "ControllerManager.h"
#include "LogManager.h"
#include "PasswordManager.h"
#include "ProgressDialog.h"
#include "ui_ApplyPage.h"
#include "ui_ConfirmSelection.h"

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
                ui->selectedDir->setText(selectedBackupDir);
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
        QMessageBox::warning(this, "오류", "제어기를 선택해주세요.");
        return;
    }

    if (selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, "오류", "백업 파일을 선택해주세요.");
        return;
    }

    for (auto cur : selectedControllerList) {
        qDebug() << "selected : " << cur;
    }

    showPasswordUI();
}

//import : 다른 백업 스페이스 폴더 찾기
void ApplyPage::importAnyspace()
{
    QString fileName =
            QFileDialog::getOpenFileName(nullptr,             // 부모 위젯 포인터
                                         "Select WorkSpace",  // 대화 상자 제목
                                         QDir::homePath(),    //초기 디렉토리 경로

                                         //표시할 파일 형식 필터
                                         //"설명 (확장자1 확장자2);;다른설명 (확장자3)"

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

            connect(
                    &confirmDialog,
                    &ConfirmSelection::applyRequested,
                    this,
                    [this]() {
                        // 비밀번호 + 최종 확인 후, 여기서부터 실제 처리 시작
                        // 1단계: 먼저 대상 제어기들의 현재 상태를 백업
                        startPreBackup();
                    },
                    Qt::SingleShotConnection);

            if (confirmDialog.exec() == QDialog::Accepted) {
            }
        } else {
            // 비밀번호 틀림
            LogManager::append("Apply password not match");
            QMessageBox::warning(this, tr("오류"), tr("비밀번호가 올바르지 않습니다."));
        }
    } else {
        // 사용자가 취소함
        LogManager::append("Password dialog cancelled");
    }
}

QString ApplyPage::determineBackupRoot() const
{
    if (selectedBackupDir.isEmpty()) {
        return QStringLiteral("C:/backup");
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

    // 안전장치: 실패 시 그냥 현재 경로 반환
    return dir.absolutePath();
}

void ApplyPage::startPreBackup()
{
    if (selectedControllerList.isEmpty()) {
        QMessageBox::warning(this, "오류", "선택된 제어기가 없습니다.");
        return;
    }

    if (preBackupInProgress_) {
        qWarning() << "[ApplyPage] Pre-backup already in progress.";
        return;
    }

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
                "오류",
                QString("등록 정보가 없는 제어기가 선택되었습니다:\n%1")
                        .arg(unknownControllers.join(", ")));
        return;
    }
    if (!disconnectedControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                "오류",
                QString("다음 제어기가 오프라인 상태입니다:\n%1")
                        .arg(disconnectedControllers.join(", ")));
        return;
    }
    if (!runningControllers.isEmpty()) {
        QMessageBox::warning(
                this,
                "오류",
                QString("제어기가 동작중입니다. 적용을 진행할 수 없습니다:\n%1")
                        .arg(runningControllers.join(", ")));
        return;
    }

    // 사전 백업 초기화
    preBackupInProgress_         = true;
    preBackupTotal_              = selectedControllerList.size();
    preBackupCompleted_          = 0;
    preBackupFailed_             = 0;
    preBackupSuccessControllers_.clear();
    preBackupFailedControllers_.clear();

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
                    QString("[ApplyPage] Pre-backup start failed: %1").arg(sn));
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
                "백업 오류",
                "적용 전에 수행할 백업 요청을 시작하지 못했습니다.\n적용을 취소합니다.");
    }
}

void ApplyPage::onPreBackupCompleted(const QString &serialNumber)
{
    if (!preBackupInProgress_)
        return;

    preBackupCompleted_++;
    preBackupSuccessControllers_.append(serialNumber);

    QString msg = QString("[ApplyPage] Pre-backup completed: %1 (%2/%3)")
                          .arg(serialNumber)
                          .arg(preBackupCompleted_)
                          .arg(preBackupTotal_);
    LogManager::append(msg);

    int doneCount = preBackupCompleted_ + preBackupFailed_;
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
                "백업 오류",
                "모든 제어기의 사전 백업이 실패했습니다.\n적용을 진행할 수 없습니다.");
        return;
    }

    // 일부만 성공한 경우:
    //   - 성공한 제어기만 apply 대상으로 사용
    //   - 실패한 제어기는 Log + 안내 메시지
    applyTargetControllers_ = preBackupSuccessControllers_;

    if (!preBackupFailedControllers_.isEmpty()) {
        QString failList = preBackupFailedControllers_.join(", ");
        QString warnMsg  = QString(
                                  "일부 제어기의 사전 백업이 실패했습니다.\n"
                                  "다음 제어기는 적용 대상에서 제외됩니다:\n%1")
                                  .arg(failList);
        QMessageBox::warning(this, "사전 백업 경고", warnMsg);

        LogManager::append(
                QString("[ApplyPage] Pre-backup failed controllers (skipped in apply): %1")
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

    QString msg = QString("[ApplyPage] Pre-backup failed: %1 (%2/%3) - %4")
                          .arg(serialNumber)
                          .arg(preBackupFailed_)
                          .arg(preBackupTotal_)
                          .arg(error);
    LogManager::append(msg);

    int doneCount = preBackupCompleted_ + preBackupFailed_;
    if (doneCount < preBackupTotal_)
        return;

    // 🔚 전체 사전 백업 응답 완료 → 최종 처리는 onPreBackupCompleted 쪽에서 공통 처리
    preBackupInProgress_ = false;

    ControllerManager *manager = ControllerManager::instance();
    disconnect(manager, &ControllerManager::backupCompleted, this, &ApplyPage::onPreBackupCompleted);
    disconnect(manager, &ControllerManager::backupFailed, this, &ApplyPage::onPreBackupFailed);

    if (preBackupSuccessControllers_.isEmpty()) {
        // 전부 실패
        QMessageBox::warning(
                this,
                "백업 실패",
                "적용 전에 수행한 사전 백업이 모두 실패했습니다.\n적용을 진행할 수 없습니다.");
        return;
    }

    // 일부 성공한 경우는 onPreBackupCompleted에서 처리되므로 여기선 따로 안 건드려도 됨
    applyTargetControllers_ = preBackupSuccessControllers_;

    if (!preBackupFailedControllers_.isEmpty()) {
        QString failList = preBackupFailedControllers_.join(", ");
        QString warnMsg  = QString(
                                  "일부 제어기의 사전 백업이 실패했습니다.\n"
                                  "다음 제어기는 적용 대상에서 제외됩니다:\n%1")
                                  .arg(failList);
        QMessageBox::warning(this, "사전 백업 경고", warnMsg);

        LogManager::append(
                QString("[ApplyPage] Pre-backup failed controllers (skipped in apply): %1")
                        .arg(failList));
    }

    startApplyQueue();
}

// 큐 시작
void ApplyPage::startApplyQueue()
{
    if (applyTargetControllers_.isEmpty() || selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, "오류", "선택된 제어기 또는 백업 파일 정보가 없습니다.");
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
        QMessageBox::warning(this,
                             "오류",
                             QString("등록 정보가 없는 제어기가 선택되었습니다:\n%1")
                                     .arg(unknownControllers.join(", ")));

        // 로그 추가
        for (const QString &sn : unknownControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is NOT REGISTERED").arg(sn);
            LogManager::append(msg);
        }

        // 전체 메시지
        {
            QString msg = QString("Apply aborted: Unknown controller(s) selected (%1)")
                                  .arg(unknownControllers.join(", "));
            LogManager::append(msg);
        }

        return;
    }

    if (!disconnectedControllers.isEmpty()) {
        QMessageBox::warning(this,
                             "오류",
                             QString("다음 제어기가 오프라인 상태입니다:\n%1")
                                     .arg(disconnectedControllers.join(", ")));

        // 로그 추가
        for (const QString &sn : disconnectedControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is OFFLINE").arg(sn);
            LogManager::append(msg);
        }

        // 전체 메시지
        {
            QString msg = QString("Apply aborted: Offline controller(s) detected (%1)")
                                  .arg(disconnectedControllers.join(", "));
            LogManager::append(msg);
        }

        return;
    }

    if (!runningControllers.isEmpty()) {
        QMessageBox::warning(this,
                             "오류",
                             QString("제어기가 동작중입니다. 전체 적용을 취소합니다:\n%1")
                                     .arg(runningControllers.join(", ")));

        // 로그 추가
        for (const auto &sn : runningControllers) {
            QString msg = QString("[%1] Apply canceled: Controller is RUNNING").arg(sn);
            LogManager::append(msg);
        }

        // 전체 적용 취소 로그
        {
            QString msg = QString("Apply aborted: %1 controller(s) running (%2)")
                                  .arg(runningControllers.size())
                                  .arg(runningControllers.join(", "));
            LogManager::append(msg);
        }
        return;
    }

    // 큐 및 카운터 초기화
    applyQueue_.clear();
    totalApplyRequests_     = selectedControllerList.size();
    completedApplyRequests_ = 0;
    failedApplyRequests_    = 0;
    applyInProgress_        = true;

    // 진행 다이얼로그 새로 생성
    if (applyProgressDialog_) {
        applyProgressDialog_->close();
        applyProgressDialog_->deleteLater();
        applyProgressDialog_ = nullptr;
    }

    applyProgressDialog_ = new ProgressDialog(this);
    applyProgressDialog_->setWindowTitle("적용 진행 중...");
    applyProgressDialog_->setTotalCount(totalApplyRequests_);
    applyProgressDialog_->setCurrentIndex(0);
    applyProgressDialog_->setSerialNumber("-");
    applyProgressDialog_->setProgress(0);
    applyProgressDialog_->setFinishedMode(false);
    applyProgressDialog_->show();

    QApplication::processEvents();

    connect(applyProgressDialog_, &ProgressDialog::cancelRequested, this, [this]() {
        if (applyProgressDialog_) {
            applyProgressDialog_->close();
        }
    });

    // Disable controls during apply
    if (ui->applyBtn)
        ui->applyBtn->setEnabled(false);

    for (const QString &sn : selectedControllerList) {
        applyQueue_.enqueue(sn);
    }

    QTimer::singleShot(0, this, &ApplyPage::processNextApply);
}

// 큐 처리
void ApplyPage::processNextApply()
{
    // 큐가 비어있으면 완료 처리
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
        onApplyFailed(serialNumber, tr("적용 요청을 시작하지 못했습니다."));
    }
}

// 개별 적용 성공
void ApplyPage::onApplyCompleted(const QString &serialNumber)
{
    if (!applyInProgress_)
        return;

    completedApplyRequests_++;
    QString msg = QString("Apply completed: %1 (%2/%3)")
                          .arg(serialNumber)
                          .arg(completedApplyRequests_)
                          .arg(totalApplyRequests_);
    LogManager::append(msg);

    int doneCount = completedApplyRequests_ + failedApplyRequests_;

    if (applyProgressDialog_ && totalApplyRequests_ > 0) {
        applyProgressDialog_->setCurrentIndex(doneCount);
        int percent = (doneCount * 100) / totalApplyRequests_;
        applyProgressDialog_->setProgress(percent);
    }

    // 다음 작업 처리
    processNextApply();
}

// 개별 적용 실패
void ApplyPage::onApplyFailed(const QString &serialNumber, const QString &error)
{
    if (!applyInProgress_)
        return;

    failedApplyRequests_++;
    qWarning() << "[ApplyPage] Apply failed:" << serialNumber << error;
    QString msg = QString("[Apply failed: %1 (%2/%3)")
                          .arg(serialNumber)
                          .arg(failedApplyRequests_)
                          .arg(totalApplyRequests_);
    LogManager::append(msg);

    int doneCount = completedApplyRequests_ + failedApplyRequests_;

    if (applyProgressDialog_ && totalApplyRequests_ > 0) {
        applyProgressDialog_->setCurrentIndex(doneCount);
        int percent = (doneCount * 100) / totalApplyRequests_;
        applyProgressDialog_->setProgress(percent);
    }

    // 다음 작업 처리
    processNextApply();
}

// 모든 Apply 완료 시 창 닫기
void ApplyPage::onAllAppliesCompleted()  //
{
    qDebug() << "[ApplyPage] All applies processed.";
    LogManager::append("All applies processed");
    applyInProgress_ = false;
    ControllerManager::instance()->resumeStateUpdates();

    if (applyProgressDialog_) {
        applyProgressDialog_->setFinishedMode(true);

        QString statusText;
        if (failedApplyRequests_ > 0) {
            statusText = QString("적용이 완료되었습니다.\n성공: %1대, 실패: %2대")
                                 .arg(completedApplyRequests_)
                                 .arg(failedApplyRequests_);
        } else {
            statusText = QString("적용이 완료되었습니다. (총 %1대, 모두 성공)")
                                 .arg(completedApplyRequests_);
        }
        applyProgressDialog_->setStatusText(statusText);
    }

    // 창을 자동으로 닫지 않고 사용자가 계속 작업할 수 있도록 유지
    // 필요 시 외부에서 직접 닫을 수 있게 한다.
    // this->window()->close();

    // Re-enable controls after processing
    if (ui->applyBtn)
        ui->applyBtn->setEnabled(true);
}

ApplyPage::~ApplyPage()
{
    delete ui;
}
