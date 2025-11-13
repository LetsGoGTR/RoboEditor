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
    QFile   file(configFile);

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
                                           tr("현재 비밀번호 확인"),
                                           tr("현재 비밀번호를 입력하세요:"),
                                           QLineEdit::Password,
                                           "",
                                           &ok);

    if (!ok || oldPwd.isEmpty())
        return;

    if (hashedPswd != hashPassword(oldPwd)) {
        QMessageBox::warning(this, tr("오류"), tr("현재 비밀번호가 올바르지 않습니다."));
        return;
    }

    QString newPwd = QInputDialog::getText(this,
                                           tr("새 비밀번호 설정"),
                                           tr("새 비밀번호를 입력하세요:"),
                                           QLineEdit::Password,
                                           "",
                                           &ok);

    if (!ok || newPwd.isEmpty())
        return;

    // 비밀번호 정책 검증 (선택사항)
    if (newPwd.length() < 4) {
        QMessageBox::warning(this, tr("오류"), tr("비밀번호는 최소 4자 이상이어야 합니다."));
        return;
    }

    QString confirmPwd = QInputDialog::getText(this,
                                               tr("새 비밀번호 확인"),
                                               tr("다시 한 번 입력하세요:"),
                                               QLineEdit::Password,
                                               "",
                                               &ok);

    if (!ok || newPwd != confirmPwd) {
        QMessageBox::warning(this, tr("오류"), tr("비밀번호가 일치하지 않습니다."));
        return;
    }

    hashedPswd = hashPassword(newPwd);
    savePasswordToConfig(hashedPswd);

    QMessageBox::information(this, tr("완료"), tr("비밀번호가 변경되었습니다."));
}

QString PasswordManager::encrypt(QString pswd)
{
    //제어기 시리얼 넘버에 저장하기, hased password : 마스터 비밀번호

    std::string master = hashedPswd.toStdString();

    unsigned char key = 0;
    for (auto i : master) {
        key ^= i;
    }
    if (key == 0)
        key = 1;

    std::string encryptedPswd = pswd.toUtf8().toStdString();
    for (int i = 0; i < pswd.size(); i++) {
        encryptedPswd[i] ^= key;
        encryptedPswd[i] = ((encryptedPswd[i] << 1) | ((encryptedPswd[i] & 0x40) >> 6)) & 0x7F;
    }

    return QString::fromUtf8(encryptedPswd.c_str());
}
QString PasswordManager::decrypt(QString pswd)
{
    std::string master = hashedPswd.toStdString();

    unsigned char key = 0;
    for (auto i : master) {
        key ^= i;
    }
    if (key == 0)
        key = 1;

    // 제어기 비밀번호 해독

    std::string decryptedPswd = pswd.toUtf8().toStdString();
    for (int i = 0; i < decryptedPswd.size(); i++) {
        decryptedPswd[i] = ((decryptedPswd[i] >> 1) | ((decryptedPswd[i] & 0x1) << 6));
        decryptedPswd[i] ^= key;
    }

    return QString::fromUtf8(decryptedPswd.c_str());
}
