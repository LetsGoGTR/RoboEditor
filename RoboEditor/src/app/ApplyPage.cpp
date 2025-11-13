#include "ApplyPage.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ConfirmSelection.h"
#include "ControllerManager.h"
#include "PasswordManager.h"
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
    qDebug() << "apply clicked";
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
        qDebug() << "Password dialog cancelled";
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

    // Disable controls during apply
    if (ui->applyBtn)
        ui->applyBtn->setEnabled(false);
    if (ui->refreshBtn)
        ui->refreshBtn->setEnabled(false);
    if (ui->importBtn)
        ui->importBtn->setEnabled(false);

    for (const QString &sn : selectedControllerList) {
        applyQueue_.enqueue(sn);
    }

    // 큐 처리 시작
    processNextApply();
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

    bool ok = ControllerManager::instance()->applyRequest(
            serialNumber, selectedBackupDir, apiPassword_);
    if (ok) {
        onApplyCompleted(serialNumber);
    } else {
        onApplyFailed(serialNumber, tr("SFTP 적용 실패"));
    }
}

// 개별 적용 성공
void ApplyPage::onApplyCompleted(const QString &serialNumber)
{
    completedApplyRequests_++;
    qDebug() << "[ApplyPage] Apply completed:" << serialNumber << "(" << completedApplyRequests_
             << "/" << totalApplyRequests_ << ")";

    // 다음 작업 처리
    processNextApply();
}

// 개별 적용 실패
void ApplyPage::onApplyFailed(const QString &serialNumber, const QString &error)
{
    failedApplyRequests_++;
    qWarning() << "[ApplyPage] Apply failed:" << serialNumber << error;

    // 다음 작업 처리
    processNextApply();
}

// 모든 Apply 완료 시 창 닫기
void ApplyPage::onAllAppliesCompleted()  //
{
    qDebug() << "[ApplyPage] All applies processed.";

    // BackupPage.cpp의 완료 로직을 참고하여 수정
    QString message;
    if (failedApplyRequests_ > 0) {
        message = QString("적용 완료!\n성공: %1개, 실패: %2개")
                          .arg(completedApplyRequests_)
                          .arg(failedApplyRequests_);
        QMessageBox::warning(this, "적용 완료", message);
    } else {
        message = QString("모든 제어기에 성공적으로 적용되었습니다!\n완료: %1개")
                          .arg(completedApplyRequests_);
        QMessageBox::information(this, "적용 완료", message);
    }

    // 창을 자동으로 닫지 않고 사용자가 계속 작업할 수 있도록 유지
    // 필요 시 외부에서 직접 닫을 수 있게 한다.
    // this->window()->close();

    // Re-enable controls after processing
    if (ui->applyBtn)
        ui->applyBtn->setEnabled(true);
    if (ui->refreshBtn)
        ui->refreshBtn->setEnabled(true);
    if (ui->importBtn)
        ui->importBtn->setEnabled(true);
}

ApplyPage::~ApplyPage()
{
    delete ui;
}
