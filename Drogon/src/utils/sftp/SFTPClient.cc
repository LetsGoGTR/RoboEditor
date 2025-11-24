#include "SFTPClient.h"

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <drogon/drogon.h>

SFTPClient::SFTPClient(const SFTPConfig &cfg) :
    config(cfg),
    socket(-1),
    session(nullptr),
    sftpSession(nullptr)
{
}

SFTPClient::~SFTPClient()
{
    disconnect();
}

bool SFTPClient::createSocket()
{
    clearError();

    // getaddrinfo를 사용하여 호스트명 또는 IP를 해석
    struct addrinfo  hints;
    struct addrinfo *result = nullptr;
    struct addrinfo *rp     = nullptr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = 0;

    // 포트를 문자열로 변환
    std::string portStr = std::to_string(config.port);

    // DNS 해석
    int gai_err = getaddrinfo(config.host.c_str(), portStr.c_str(), &hints, &result);
    if (gai_err != 0) {
        lastError = "호스트명 해석 실패: " + config.host + " (" +
                    std::string(gai_strerror(gai_err)) + ")";
        LOG_ERROR << "DNS 해석 실패: " << config.host << " - "
                  << gai_strerror(gai_err);
        return false;
    }

    // 결과 중 첫 번째 주소로 소켓 생성 및 연결 시도
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socket = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket < 0) {
            continue;  // 다음 시도
        }

        if (::connect(socket, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // 연결 성공
        }

        ::close(socket);
        socket = -1;
    }

    freeaddrinfo(result);

    if (rp == nullptr) {
        lastError = "SSH 서버 연결 실패 (호스트: " + config.host +
                    ", 포트: " + std::to_string(config.port) + ")";
        LOG_ERROR << "SFTP 서버 연결 실패: " << config.host << ":"
                  << config.port;
        socket = -1;
        return false;
    }

    LOG_INFO << "소켓 연결 성공: " << config.host << ":" << config.port;
    return true;
}

bool SFTPClient::authenticatePassword()
{
    clearError();
    if (libssh2_userauth_password(session, config.user.c_str(), config.password.c_str()) != 0) {
        lastError = "인증 실패 (사용자명: " + config.user + ")";
        LOG_ERROR << "SFTP 인증 실패: " << config.user;
        return false;
    }
    return true;
}

bool SFTPClient::initSFTP()
{
    clearError();
    sftpSession = libssh2_sftp_init(session);
    if (!sftpSession) {
        lastError = "SFTP 세션 초기화 실패";
        LOG_ERROR << "SFTP 세션 초기화 실패";
        return false;
    }
    return true;
}

bool SFTPClient::connectSSHOnly()
{
    clearError();

    // 1. 소켓 생성
    if (!createSocket()) {
        return false;
    }

    // 2. SSH 세션 초기화
    session = libssh2_session_init();
    if (!session) {
        lastError = "SSH 세션 초기화 실패";
        LOG_ERROR << "SSH 세션 초기화 실패";
        ::close(socket);
        socket = -1;
        return false;
    }

    // 3. SSH 핸드셰이크
    if (libssh2_session_handshake(session, socket) != 0) {
        lastError = "SSH 핸드셰이크 실패";
        LOG_ERROR << "SSH 핸드셰이크 실패";
        libssh2_session_free(session);
        ::close(socket);
        session = nullptr;
        socket  = -1;
        return false;
    }

    // 4. 인증
    if (!authenticatePassword()) {
        libssh2_session_free(session);
        ::close(socket);
        session = nullptr;
        socket  = -1;
        return false;
    }

    LOG_INFO << "SSH 연결 성공: " << config.host << ":" << config.port;
    return true;
}

bool SFTPClient::connect()
{
    // SSH 연결
    if (!connectSSHOnly()) {
        return false;
    }

    // SFTP 초기화
    if (!initSFTP()) {
        libssh2_session_free(session);
        ::close(socket);
        session = nullptr;
        socket  = -1;
        return false;
    }

    LOG_INFO << "SFTP 연결 성공: " << config.host << ":" << config.port;
    return true;
}

void SFTPClient::disconnect()
{
    if (sftpSession) {
        libssh2_sftp_shutdown(sftpSession);
        sftpSession = nullptr;
    }
    if (session) {
        libssh2_session_disconnect(session, "정상 종료");
        libssh2_session_free(session);
        session = nullptr;
    }
    if (socket >= 0) {
        ::close(socket);
        socket = -1;
    }
}

bool SFTPClient::isConnected() const
{
    return socket >= 0 && session != nullptr && sftpSession != nullptr;
}

bool SFTPClient::uploadFile(const std::string &localPath, const std::string &remotePath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        LOG_ERROR << "SFTP 업로드 실패: 연결 상태 아님";
        return false;
    }

    std::ifstream localFile(localPath, std::ios::binary);
    if (!localFile) {
        lastError = "로컬 파일 열기 실패: " + localPath;
        LOG_ERROR << "로컬 파일 열기 실패: " << localPath;
        return false;
    }

    LIBSSH2_SFTP_HANDLE *remoteFile =
            libssh2_sftp_open(sftpSession,
                              remotePath.c_str(),
                              LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                              0644);

    if (!remoteFile) {
        lastError = "원격 파일 생성 실패: " + remotePath;
        LOG_ERROR << "원격 파일 생성 실패: " << remotePath;
        return false;
    }

    char buffer[16384];
    bool success = true;

    while (localFile.read(buffer, sizeof(buffer)) || localFile.gcount() > 0) {
        ssize_t nwritten = libssh2_sftp_write(remoteFile, buffer, localFile.gcount());
        if (nwritten < 0 || nwritten != localFile.gcount()) {
            lastError = "파일 쓰기 오류: " + remotePath;
            LOG_ERROR << "파일 쓰기 오류: " << remotePath;
            success = false;
            break;
        }
    }

    libssh2_sftp_close_handle(remoteFile);
    if (success) {
        LOG_INFO << "파일 업로드 성공: " << localPath << " -> " << remotePath;
    }
    return success;
}

bool SFTPClient::downloadFile(const std::string &remotePath, const std::string &localPath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        LOG_ERROR << "SFTP 다운로드 실패: 연결 상태 아님";
        return false;
    }

    LIBSSH2_SFTP_HANDLE *remoteFile =
            libssh2_sftp_open(sftpSession, remotePath.c_str(), LIBSSH2_FXF_READ, 0);

    if (!remoteFile) {
        lastError = "원격 파일 열기 실패: " + remotePath;
        LOG_ERROR << "원격 파일 열기 실패: " << remotePath;
        return false;
    }

    std::ofstream localFile(localPath, std::ios::binary);
    if (!localFile) {
        lastError = "로컬 파일 생성 실패: " + localPath;
        LOG_ERROR << "로컬 파일 생성 실패: " << localPath;
        libssh2_sftp_close_handle(remoteFile);
        return false;
    }

    char    buffer[16384];
    ssize_t nbytes;
    bool    success = true;

    while ((nbytes = libssh2_sftp_read(remoteFile, buffer, sizeof(buffer))) > 0) {
        localFile.write(buffer, nbytes);
    }

    if (nbytes < 0) {
        lastError = "파일 읽기 오류: " + remotePath;
        LOG_ERROR << "파일 읽기 오류: " << remotePath;
        success = false;
    }

    libssh2_sftp_close_handle(remoteFile);
    if (success) {
        LOG_INFO << "파일 다운로드 성공: " << remotePath << " -> " << localPath;
    }
    return success;
}

bool SFTPClient::deleteFile(const std::string &remotePath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        return false;
    }

    if (libssh2_sftp_unlink(sftpSession, remotePath.c_str()) != 0) {
        lastError = "파일 삭제 실패: " + remotePath;
        LOG_ERROR << "파일 삭제 실패: " << remotePath;
        return false;
    }

    LOG_INFO << "파일 삭제 성공: " << remotePath;
    return true;
}

bool SFTPClient::createDirectory(const std::string &remotePath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        return false;
    }

    if (libssh2_sftp_mkdir(sftpSession, remotePath.c_str(), 0755) != 0) {
        lastError = "디렉토리 생성 실패: " + remotePath;
        LOG_ERROR << "디렉토리 생성 실패: " << remotePath;
        return false;
    }

    LOG_INFO << "디렉토리 생성 성공: " << remotePath;
    return true;
}

std::vector<std::string> SFTPClient::listDirectory(const std::string &remotePath)
{
    std::vector<std::string> files;
    clearError();

    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        return files;
    }

    LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_open_ex(
            sftpSession, remotePath.c_str(), remotePath.length(), 0, 0, LIBSSH2_SFTP_OPENDIR);

    if (!handle) {
        lastError = "디렉토리 열기 실패: " + remotePath;
        LOG_ERROR << "디렉토리 열기 실패: " << remotePath;
        return files;
    }

    char                    mem[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (libssh2_sftp_readdir(handle, mem, sizeof(mem), &attrs) > 0) {
        files.push_back(std::string(mem));
    }

    libssh2_sftp_close_handle(handle);
    return files;
}

void SFTPClient::clearError()
{
    lastError.clear();
}

bool SFTPClient::isFile(const std::string &remotePath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_stat(sftpSession, remotePath.c_str(), &attrs) != 0) {
        lastError = "원격 파일 정보 조회 실패: " + remotePath;
        return false;
    }

    return LIBSSH2_SFTP_S_ISREG(attrs.permissions);
}

bool SFTPClient::isDirectory(const std::string &remotePath)
{
    clearError();
    if (!isConnected()) {
        lastError = "SSH 연결 상태가 아닙니다";
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_stat(sftpSession, remotePath.c_str(), &attrs) != 0) {
        lastError = "원격 디렉토리 정보 조회 실패: " + remotePath;
        return false;
    }

    return LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
}

bool SFTPClient::executeCommand(const std::string &command, std::string &output)
{
    clearError();
    if (socket < 0 || session == nullptr) {
        lastError = "SSH 연결 상태가 아닙니다";
        return false;
    }

    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        lastError = "채널 오픈 실패";
        LOG_ERROR << "SSH 채널 오픈 실패";
        return false;
    }

    // 명령 실행
    if (libssh2_channel_exec(channel, command.c_str()) != 0) {
        lastError = "명령 실행 실패: " + command;
        LOG_ERROR << "SSH 명령 실행 실패: " << command;
        libssh2_channel_free(channel);
        return false;
    }

    // 출력 읽기
    char    buffer[4096];
    ssize_t nbytes;
    output.clear();

    while ((nbytes = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, nbytes);
    }

    // 에러 스트림 읽기
    std::string errorOutput;
    while ((nbytes = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer))) > 0) {
        errorOutput.append(buffer, nbytes);
    }

    // 종료 상태 확인
    int exitStatus = 0;
    libssh2_channel_wait_closed(channel);
    exitStatus = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);

    if (exitStatus != 0) {
        lastError = "명령 실행 실패 (exit code: " + std::to_string(exitStatus) + ")";
        if (!errorOutput.empty()) {
            lastError += ": " + errorOutput;
        }
        LOG_ERROR << "SSH 명령 실행 실패: " << command
                  << " (exit: " << exitStatus << ")"
                  << (errorOutput.empty() ? "" : " stderr: " + errorOutput);
        return false;
    }

    LOG_INFO << "SSH 명령 실행 성공: " << command;
    return true;
}
