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
        QString encrypt(QString pswd);
        QString decrypt(QString pswd);

      private:
        Ui::PasswordInput *ui;
        QString            hashedPswd;

        //master key
        QString hashPassword(const QString &pwd) const;
        void    loadPasswordFromConfig();
        void    savePasswordToConfig(const QString &hashedPwd);

        QString getConfigFilePath() const;

      public slots:
        void changePassword();
};

#endif  // PASSWORDMANAGER_H
