// PasswordManager.h
#ifndef PASSWORDMANAGER_H
#define PASSWORDMANAGER_H
#pragma once
#include <QDialog>
#include <QString>

namespace Ui
{
    class PasswordInput;
}

class PasswordManager : public QDialog
{
    Q_OBJECT

      public:
        explicit PasswordManager(QWidget *parent = nullptr);
        ~PasswordManager();

        QString getMasterPassword() const;
        bool    isCorrect(const QString &input) const;

        //제어기 password
        QByteArray deriveKey() const;
        QString    encrypt(QString pswd);
        QString    decrypt(const QString &pswd);

        void setHashedPassword(const QString &hashed)
        {
            hashedPswd = hashed;
        }

        QString hashPassword(const QString &pwd) const;
        void    loadPasswordFromConfig();
        void    savePasswordToConfig(const QString &hashedPwd);

      private:
        Ui::PasswordInput *ui;
        QString            hashedPswd;

        //master key

        QString getConfigFilePath() const;
      signals:
        void masterPasswordChanged();

      public slots:
        void changePassword();
};
void testPasswordEncryption();
#endif  // PASSWORDMANAGER_H
