#include "ControllerSetting.h"

#include <QMessageBox>

#include "ui_ControllerSettings.h"

ControllerSetting::ControllerSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControllerSettings)
{
    ui->setupUi(this);

    connect(ui->OKBtn, &QPushButton::clicked, this, &ControllerSetting::onAcceptBtn);
    connect(ui->CancelBtn, &QPushButton::clicked, this, &ControllerSetting::onRejectBtn);

    ui->WPBox->setText("/home/default");
    ui->sftpSpinBox->setValue(22);
    ui->sftpSpinBox->setRange(1, 65535);

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

    QString sn     = ui->SNBox->text().trimmed();
    QString ip     = ui->ipBox->text().trimmed();
    QString user   = ui->usernameBox->text().trimmed();
    QString pswd   = ui->pswdBox->text().trimmed();
    QString wsPath = ui->WPBox->text().trimmed();
    int     sftp   = ui->sftpSpinBox->value();

    if (sn.isEmpty() || ip.isEmpty() || user.isEmpty() || wsPath.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "모든 항목을 입력해야 합니다.");
        return ControllerInfo();  // 빈 구조체
    }

    info.serialNumber = ui->SNBox->text().trimmed();
    info.ip           = ui->ipBox->text().trimmed();
    info.username     = ui->usernameBox->text().trimmed();
    info.sftpPort     = ui->sftpSpinBox->value();
    info.pswd         = ui->pswdBox->text().trimmed();
    info.wsPath       = ui->WPBox->text().trimmed();
    info.isConnected  = m_info.isConnected;
    info.isRunning    = m_info.isRunning;

    return info;
}
// 객체 -> ui 세팅
void ControllerSetting::setControllerInfo(const ControllerInfo &info)
{
    m_info = info;

    ui->SNBox->setText(info.serialNumber);
    ui->ipBox->setText(info.ip);
    ui->usernameBox->setText(info.username);
    ui->sftpSpinBox->setValue(info.sftpPort);
    ui->WPBox->setText(info.wsPath);
    ui->pswdBox->setText(info.pswd);
}
