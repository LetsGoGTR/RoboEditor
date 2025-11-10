#include "SftpClient.h"

#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>

SFTPClient::SFTPClient(
        const QString &host, int port, const QString &user, const QString &pass, QObject *parent) :
    QObject(parent),
    host_(host),
    port_(port),
    user_(user),
    pass_(pass)
{
    int rc = libssh2_init(0);

    qDebug() << "========== libssh2 Capabilities ==========";
    qDebug() << "Version:" << libssh2_version(0);

    // 임시 세션을 만들어서 지원 알고리즘 확인
    LIBSSH2_SESSION *temp_session = libssh2_session_init();
    if (temp_session) {
        // 기본 지원 알고리즘 목록 확인
        const char *kex     = libssh2_session_methods(temp_session, LIBSSH2_METHOD_KEX);
        const char *hostkey = libssh2_session_methods(temp_session, LIBSSH2_METHOD_HOSTKEY);
        const char *crypt   = libssh2_session_methods(temp_session, LIBSSH2_METHOD_CRYPT_CS);
        const char *mac     = libssh2_session_methods(temp_session, LIBSSH2_METHOD_MAC_CS);

        qDebug() << "Supported KEX:" << (kex ? kex : "NULL");
        qDebug() << "Supported HOSTKEY:" << (hostkey ? hostkey : "NULL");
        qDebug() << "Supported CRYPT:" << (crypt ? crypt : "NULL");
        qDebug() << "Supported MAC:" << (mac ? mac : "NULL");

        libssh2_session_free(temp_session);
    }
    qDebug() << "==========================================";
}

SFTPClient::~SFTPClient()
{
    disconnect();
    libssh2_exit();
}
bool SFTPClient::connectToServer()
{
    socket_.connectToHost(host_, port_);
    if (!socket_.waitForConnected(5000)) {
        qWarning() << "TCP connect failed:" << socket_.errorString();
        return false;
    }

    // 네이티브 소켓 핸들 얻기
    qintptr          fd     = socket_.socketDescriptor();
    libssh2_socket_t sockfd = static_cast<libssh2_socket_t>(fd);

    // libssh2 세션 초기화
    session_ = libssh2_session_init();
    libssh2_session_set_blocking(session_, 1);

    // SSH 핸드쉐이크
    int rc = libssh2_session_handshake(session_, sockfd);
    if (rc) {
        qWarning() << "Handshake failed:" << rc;
        return false;
    }

    // 사용자 인증
    rc = libssh2_userauth_password(
            session_, user_.toUtf8().constData(), pass_.toUtf8().constData());
    if (rc) {
        qWarning() << "Authentication failed:" << rc;
        return false;
    }
    //4 SFTP 세션 생성(파일 전송용 채널)

    sftpSession_ = libssh2_sftp_init(session_);
    if (!sftpSession_) {
        qWarning() << "Unable to init SFTP session";
        return false;
    }

    return true;
}
void SFTPClient::disconnect()
{
    // 1. SFTP 세션 종료
    if (sftpSession_) {
        libssh2_sftp_shutdown(sftpSession_);
        sftpSession_ = nullptr;
        qDebug() << "[SFTPClient] SFTP session shutdown.";
    }

    // 2. SSH 세션 종료
    if (session_) {
        libssh2_session_disconnect(session_, "Normal Shutdown");
        libssh2_session_free(session_);
        session_ = nullptr;
        qDebug() << "[SFTPClient] SSH session disconnected.";
    }

    // 3. TCP 소켓 닫기
    if (socket_.isOpen()) {
        socket_.disconnectFromHost();
        if (socket_.state() != QAbstractSocket::UnconnectedState)
            socket_.waitForDisconnected(3000);
        qDebug() << "[SFTPClient] TCP socket closed.";
    }
}

bool SFTPClient::uploadFile(const QString &localPath, const QString &remotePath)
{
    //sftp 세션 확인
    if (!sftpSession_) {
        qWarning() << "SFTP session not initialized";
        return false;
    }

    // 파일 열어지는지 확인
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open local file:" << localPath;
        return false;
    }

    LIBSSH2_SFTP_HANDLE *sftpHandle =
            libssh2_sftp_open(sftpSession_,
                              remotePath.toUtf8().constData(),

                              LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                              LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP |
                                      LIBSSH2_SFTP_S_IROTH);
    if (!sftpHandle) {
        qWarning() << "Unable to open remote file:" << remotePath
                   << "Error:" << libssh2_sftp_last_error(sftpSession_);
        return false;
    }

    char   buffer[8192];
    qint64 totalWritten = 0;
    bool   success      = true;

    while (!localFile.atEnd()) {
        qint64 bytesRead = localFile.read(buffer, sizeof(buffer));
        if (bytesRead <= 0)
            break;

        char  *ptr       = buffer;
        qint64 remaining = bytesRead;

        while (remaining > 0) {
            ssize_t written = libssh2_sftp_write(sftpHandle, ptr, remaining);
            if (written < 0) {
                qWarning() << "SFTP write error:" << written;
                success = false;
                break;
            }
            ptr += written;
            remaining -= written;
            totalWritten += written;
        }

        if (!success)
            break;
    }

    libssh2_sftp_close(sftpHandle);
    localFile.close();

    if (success) {
        qDebug() << "Uploaded" << totalWritten << "bytes to" << remotePath;
    }

    return success;
}
bool SFTPClient::downloadFile(const QString &remotePath, const QString &localPath)
{
    if (!sftpSession_) {
        qWarning() << "SFTP session not initialized";
        return false;
    }

    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open local file:" << localPath;
        return false;
    }

    LIBSSH2_SFTP_HANDLE *sftpHandle = libssh2_sftp_open(sftpSession_,
                                                        remotePath.toUtf8().constData(),

                                                        LIBSSH2_FXF_READ,
                                                        0);
    if (!sftpHandle) {
        qWarning() << "Unable to open remote file:" << remotePath
                   << "Error:" << libssh2_sftp_last_error(sftpSession_);
        return false;
    }
    char   buffer[8192];
    qint64 totalRead = 0;
    bool   success   = true;

    while (true) {
        ssize_t bytesRead = libssh2_sftp_read(sftpHandle, buffer, sizeof(buffer));

        if (bytesRead < 0) {
            qWarning() << "SFTP read error:" << bytesRead;
            success = false;
            break;
        }

        if (bytesRead == 0)
            break;  // EOF

        // 파일에 쓰기!
        qint64 written = localFile.write(buffer, bytesRead);
        if (written != bytesRead) {
            qWarning() << "Local write error";
            success = false;
            break;
        }

        totalRead += bytesRead;
    }

    libssh2_sftp_close(sftpHandle);  // handle만 닫기
    localFile.close();
    if (success) {
        qDebug() << "Downloaded" << totalRead << "bytes to" << localPath;
    }

    return success;
}
