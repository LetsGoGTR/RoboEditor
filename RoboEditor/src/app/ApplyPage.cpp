#include "ApplyPage.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "ConfirmSelection.h"
#include "PasswordManager.h"
#include "ui_ApplyPage.h"
#include "ui_ConfirmSelection.h"

ApplyPage::ApplyPage(QWidget *parent) : QWidget(parent), ui(new Ui::ApplyPage)
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
    qDebug() << "selected : " << selectedBackupDir;

    for (auto cur : selectedControllerList) {
        qDebug() << "selected : " << cur;
    }
    selectedControllerList = selectcontrollerWidget->getSelectedControllers();
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
        QString inputPassword = manager.getPassword();
        if (manager.isCorrect(inputPassword)) {
            // 비밀번호 검증 성공
            ConfirmSelection confirmDialog(this);
            confirmDialog.setApplyPage(this);

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

ApplyPage::~ApplyPage()
{
    delete ui;
}
