#include "PasswordManager.h"

#include <QTextStream>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include "ControllerManager.h"
#include "ui_PasswordInput.h"

PasswordManager::PasswordManager(QWidget *parent) : QDialog(parent), ui(new Ui::PasswordInput)
{
    ui->setupUi(this);
    loadPasswordFromConfig();

    QDialogButtonBox *buttonBox = this->findChild<QDialogButtonBox *>();
    if (buttonBox) {
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    } else {
        qWarning() << "QDialogButtonBox not found in PasswordInput UI!";
    }
}

PasswordManager::~PasswordManager()
{
    delete ui;
}

QString PasswordManager::getMasterPassword() const
{
    return ui->passwordLineEdit->text();
}

QString PasswordManager::hashPassword(const QString &pwd) const
{
    return QString(QCryptographicHash::hash(pwd.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString PasswordManager::getConfigFilePath() const
{
    // 애플리케이션 데이터 저장 경로

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    qDebug() << configPath;
    // 디렉토리가 없으면 생성
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return configPath + "/password.conf";
}

void PasswordManager::loadPasswordFromConfig()
{
    QString configFile = getConfigFilePath();
    QFile   file(configFile);

    if (!file.exists()) {
        // 설정 파일이 없으면 기본 비밀번호로 초기화
        hashedPswd = hashPassword("0000");
        savePasswordToConfig(hashedPswd);
        qDebug() << "설정 파일이 없습니다. 기본 비밀번호(0000)로 초기화합니다.";
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "설정 파일을 열 수 없습니다:" << configFile;
        hashedPswd = hashPassword("0000");
        return;
    }

    QTextStream in(&file);
    QString     line = in.readLine();
    file.close();

    // 파일에서 읽은 해시값 검증 (SHA256은 64자 hex)
    if (line.length() == 64) {
        hashedPswd = line;
        qDebug() << "설정 파일에서 비밀번호를 로드했습니다.";
    } else {
        qWarning() << "설정 파일의 형식이 올바르지 않습니다.";
        hashedPswd = hashPassword("0000");
        savePasswordToConfig(hashedPswd);
    }
}

void PasswordManager::savePasswordToConfig(const QString &hashedPwd)
{
    QString configFile = getConfigFilePath();

    QFile file(configFile);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "설정 파일에 쓸 수 없습니다:" << configFile;
        return;
    }

    QTextStream out(&file);
    out << hashedPwd;
    file.close();

#ifdef Q_OS_UNIX
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
#endif

    qDebug() << "비밀번호가 저장되었습니다:" << configFile;
}

bool PasswordManager::isCorrect(const QString &input) const
{
    return hashedPswd == hashPassword(input);
}

void PasswordManager::changePassword()
{
    bool    ok;
    QString oldPwd = QInputDialog::getText(this,
                                           tr("Verify Current Password"),
                                           tr("Enter current password:"),
                                           QLineEdit::Password,
                                           "",
                                           &ok);

    if (!ok || oldPwd.isEmpty())
        return;

    if (hashedPswd != hashPassword(oldPwd)) {
        QMessageBox::warning(this, tr("Error"), tr("Incorrect current password."));
        return;
    }

    QString newPwd = QInputDialog::getText(this,
                                           tr("Set New Password"),
                                           tr("Enter new password:"),
                                           QLineEdit::Password,
                                           "",
                                           &ok);

    if (!ok || newPwd.isEmpty())
        return;

    // 비밀번호 정책 검증 (선택사항)
    if (newPwd.length() < 4) {
        QMessageBox::warning(this, tr("Error"), tr("Password must be at least 4 characters long."));
        return;
    }

    QString confirmPwd = QInputDialog::getText(this,
                                               tr("Confirm New Password"),
                                               tr("Re-enter new password:"),
                                               QLineEdit::Password,
                                               "",
                                               &ok);

    if (!ok || newPwd != confirmPwd) {
        QMessageBox::warning(this, tr("Error"), tr("Passwords do not match."));
        return;
    }

    hashedPswd = hashPassword(newPwd);
    savePasswordToConfig(hashedPswd);

    //신호 발생 : 제어기 config에 있는 비밀번호 업데이트
    emit masterPasswordChanged();

    QMessageBox::information(this, tr("Success"), tr("Password changed successfully."));
}
QByteArray PasswordManager::deriveKey() const
{
    // hashedPswd(64 hex) → raw 32 bytes
    return QByteArray::fromHex(hashedPswd.toUtf8());
}
QString PasswordManager::encrypt(QString pswd)
{
    QByteArray key = deriveKey();
    if (key.isEmpty())
        return QString();

    QByteArray data = pswd.toUtf8();
    QByteArray out  = data;

    for (int i = 0; i < data.size(); i++) {
        out[i] = data[i] ^ key[i % key.size()];
    }

    return QString(out.toBase64());
}
QString PasswordManager::decrypt(const QString &encrypted)
{
    QByteArray key = deriveKey();
    if (key.isEmpty())
        return QString();

    QByteArray data = QByteArray::fromBase64(encrypted.toUtf8());
    QByteArray out  = data;

    for (int i = 0; i < data.size(); i++) {
        out[i] = data[i] ^ key[i % key.size()];
    }

    return QString::fromUtf8(out);
}
