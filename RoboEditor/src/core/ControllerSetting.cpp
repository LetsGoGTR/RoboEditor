#include "ControllerSetting.h"

#include <QMessageBox>

#include "ui_ControllerSettings.h"

ControllerSetting::ControllerSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControllerSettings)
{
    ui->setupUi(this);

    ui->ProtocolGroup->setId(ui->HTTPBtn, 0);   // HTTP = 0
    ui->ProtocolGroup->setId(ui->HTTPSBtn, 1);  // HTTPS = 1

    connect(ui->OKBtn, &QPushButton::clicked, this, &ControllerSetting::onAcceptBtn);
    connect(ui->CancelBtn, &QPushButton::clicked, this, &ControllerSetting::onRejectBtn);

    ui->sftpSpinBox->setValue(22);
    ui->sftpSpinBox->setRange(1, 65535);

    ui->apiSpinBox->setValue(80);
    ui->apiSpinBox->setRange(1, 65535);

    ui->pswdBox->setEchoMode(QLineEdit::Password);
}

ControllerSetting::~ControllerSetting()
{
    delete ui;
}
void ControllerSetting::onAcceptBtn()
{
    // 유효성 검사
    ControllerInfo info = getControllerInfo();
    if (info.serialNumber.isEmpty()) {
        return;
    }
    m_info = info;
    accept();
}
void ControllerSetting::onRejectBtn()
{
    reject();
}

// ui-> 객체 저장
ControllerInfo ControllerSetting::getControllerInfo()
{
    ControllerInfo info;

    QString sn   = ui->SNBox->text().trimmed();
    QString host = ui->hostBox->text().trimmed();
    QString user = ui->usernameBox->text().trimmed();
    QString pswd = ui->pswdBox->text().trimmed();

    if (sn.isEmpty() || host.isEmpty() || user.isEmpty() || pswd.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "모든 항목을 입력해야 합니다.");
        return ControllerInfo();  // 빈 구조체
    }

    info.serialNumber = ui->SNBox->text().trimmed();
    info.protocol     = ui->ProtocolGroup->checkedId();
    info.host         = ui->hostBox->text().trimmed();
    info.username     = ui->usernameBox->text().trimmed();
    info.apiPort      = ui->apiSpinBox->value();
    info.sftpPort     = ui->sftpSpinBox->value();
    info.pswd         = ui->pswdBox->text().trimmed();
    info.wsPath       = QString("/home/%1").arg(info.username);
    info.isConnected  = m_info.isConnected;
    info.isRunning    = m_info.isRunning;
    return info;
}
// 객체 -> ui 세팅
void ControllerSetting::setControllerInfo(const ControllerInfo &info)
{
    m_info = info;

    ui->SNBox->setText(info.serialNumber);
    ui->hostBox->setText(info.host);

    QAbstractButton *button = ui->ProtocolGroup->button(info.protocol);
    if (button) {
        button->setChecked(true);
    }

    ui->apiSpinBox->setValue(info.apiPort);
    ui->sftpSpinBox->setValue(info.sftpPort);
    ui->usernameBox->setText(info.username);
    ui->pswdBox->setText(info.pswd);
}
