#ifndef SFTPCLIENT_H
#define SFTPCLIENT_H
#pragma once
#include <QTcpSocket>

#include <QFile>
#include <QObject>

#include <libssh2.h>
#include <libssh2_sftp.h>

class SFTPClient : public QObject
{
    Q_OBJECT
      public:
        SFTPClient(const QString &host,
                   int            port,
                   const QString &user,
                   const QString &pass,
                   QObject       *parent = nullptr);
        ~SFTPClient();

        bool connectToServer();
        void disconnect();

        bool    uploadFile(const QString &localPath, const QString &remotePath);
        bool    downloadFile(const QString &remotePath, const QString &localPath);
        QString getLastError() const
        {
            return lastError_;
        }

      private:
        QString host_;
        int     port_;
        QString user_;
        QString pass_;
        QString lastError_;

        QTcpSocket       socket_;
        LIBSSH2_SESSION *session_     = nullptr;
        LIBSSH2_SFTP    *sftpSession_ = nullptr;
};

#endif  // SFTPCLIENT_H
