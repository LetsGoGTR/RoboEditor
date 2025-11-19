#pragma once
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <string>
#include <vector>

#include "SFTPConfig.h"

class SFTPClient
{
  public:
    SFTPClient(const SFTPConfig &config);
    ~SFTPClient();

    // 연결 관리
    bool connect();         // SSH + SFTP 연결
    bool connectSSHOnly();  // SSH만 연결 (명령 실행용)
    bool initSFTP();        // SFTP 세션 초기화 (connectSSHOnly 이후 호출)
    void disconnect();
    bool isConnected() const;

    // SFTP 작업
    bool uploadFile(const std::string &localPath, const std::string &remotePath);
    bool downloadFile(const std::string &remotePath, const std::string &localPath);
    bool deleteFile(const std::string &remotePath);
    bool createDirectory(const std::string &remotePath);
    std::vector<std::string> listDirectory(const std::string &remotePath);

    // 파일/디렉토리 타입 확인
    bool isFile(const std::string &remotePath);
    bool isDirectory(const std::string &remotePath);

    // SSH 명령 실행
    bool executeCommand(const std::string &command, std::string &output);

    // 상태 조회
    std::string getLastError() const
    {
        return lastError;
    }

  private:
    SFTPConfig       config;
    int              socket;
    LIBSSH2_SESSION *session;
    LIBSSH2_SFTP    *sftpSession;
    std::string      lastError;

    bool createSocket();
    bool authenticatePassword();
    void clearError();
};
