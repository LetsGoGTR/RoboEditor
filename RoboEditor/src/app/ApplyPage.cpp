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
                        // 비밀번호 + 최종 확인 후, 여기서부터 실제 Apply 시작
                        startApplyQueue();
                    },
                    Qt::SingleShotConnection);

            if (confirmDialog.exec() == QDialog::Accepted) {
                qDebug() << "User confirmed";
            }
        } else {
            // 비밀번호 틀림

            QMessageBox::warning(this, tr("오류"), tr("비밀번호가 올바르지 않습니다."));
        }
    } else {
        // 사용자가 취소함
        LogManager::append("Password dialog cancelled");
    }
}

// 큐 시작
void ApplyPage::startApplyQueue()
{
    if (selectedControllerList.isEmpty() || selectedBackupDir.isEmpty()) {
        QMessageBox::warning(this, "오류", "선택된 제어기 또는 백업 파일 정보가 없습니다.");
        return;
    }

    ControllerManager *manager = ControllerManager::instance();
    QStringList        disconnectedControllers;
    QStringList        unknownControllers;
    QStringList        runningControllers;

    manager->pauseStateUpdates();

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
        QMessageBox::warning(this,
                             "오류",
                             QString("등록 정보가 없는 제어기가 선택되었습니다:\n%1")
                                     .arg(unknownControllers.join(", ")));
        return;
    }

    if (!disconnectedControllers.isEmpty()) {
        QMessageBox::warning(this,
                             "오류",
                             QString("다음 제어기가 오프라인 상태입니다:\n%1")
                                     .arg(disconnectedControllers.join(", ")));
        return;
    }

    if (!runningControllers.isEmpty()) {
        QMessageBox::warning(this,
                             "오류",
                             QString("제어기가 동작중입니다. 전체 적용을 취소합니다:\n%1")
                                     .arg(runningControllers.join(", ")));
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
