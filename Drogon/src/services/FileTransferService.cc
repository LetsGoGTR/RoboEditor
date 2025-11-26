#include "FileTransferService.h"

#include <algorithm>
#include <bcrypt.h>
#include <ctime>
#include <drogon/HttpClient.h>
#include <drogon/utils/Utilities.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <future>

#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/ConfigUtils.h"
#include "../utils/TimeUtils.h"
#include "../utils/connection/ConnectionValidator.h"
#include "../utils/device/DeviceMetadataHelper.h"
#include <drogon/drogon.h>

namespace fs = std::filesystem;

services::ServiceResult FileTransferService::backupFromRemote(const std::string &deviceId)
{
    try {
        LOG_INFO << "원격 백업 다운로드 시작: deviceId=" << deviceId;

        // 1. Device 정보 조회
        auto deviceResult = services::DeviceService::readDevice(deviceId);
        if (!deviceResult.success) {
            return services::ServiceResult::createError("Device not found: " + deviceId);
        }

        // 2. Device metadata에서 SFTP 및 API 정보 추출
        utils::ConnectionParams connParams;
        auto extractResult =
                utils::DeviceMetadataHelper::extractConnectionParamsFromJson(deviceResult.data,
                                                                              connParams);
        if (!extractResult.success) {
            return services::ServiceResult::createError("Invalid device metadata: " +
                                                        extractResult.errorMessage);
        }

        std::string sftpHost = connParams.sftpHost;
        std::string sftpUser = connParams.sftpUser;
        std::string apiUrl   = connParams.apiUrl;

        LOG_INFO << "Device info: apiUrl=" << apiUrl << ", sftpHost=" << sftpHost
                             << ", sftpUser=" << sftpUser;

        // 3. 원격 서버의 workspace compress API 호출
        LOG_INFO << "원격 서버 압축 API 호출: " << apiUrl << "/api/workspace/compress";

        auto compressResult = compressWorkspace(sftpUser, apiUrl);
        if (!compressResult.success) {
            std::string message = compressResult.errorMessage;
            if (!compressResult.remoteMessage.empty()) {
                message += ": " + compressResult.remoteMessage;
            }
            return services::ServiceResult::createError(message);
        }

        // 4. SFTP 연결
        SFTPConfig sftpConfig = utils::DeviceMetadataHelper::createSFTPConfig(connParams);
        SFTPClient sftpClient(sftpConfig);
        if (!sftpClient.connect()) {
            return services::ServiceResult::createError(sftpClient.getLastError());
        }

        // 5. 원격 파일 경로 구성 (output.tgz 고정)
        std::string fullRemotePath = "/home/" + sftpUser + "/output.tgz";
        LOG_INFO << "원격 파일 다운로드: " << fullRemotePath;

        // 6. 임시 디렉토리에 다운로드
        std::string tempDir = "/tmp/backup_";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string tempFile = tempDir + drogon::utils::getUuid() + ".tar.gz";

        // 7. SFTP로 파일 다운로드
        LOG_INFO << "파일 다운로드 시작: " << fullRemotePath << " -> " << tempFile;
        if (!sftpClient.downloadFile(fullRemotePath, tempFile)) {
            return services::ServiceResult::createError("파일 다운로드 실패: " +
                                                        sftpClient.getLastError());
        }

        LOG_INFO << "파일 다운로드 성공: " << tempFile;

        // 8. 다운로드한 파일이 존재하는지 확인
        if (!fs::exists(tempFile) || !fs::is_regular_file(tempFile)) {
            return services::ServiceResult::createError("다운로드한 파일이 존재하지 않음: " +
                                                        tempFile);
        }

        // 9. workspace 메타데이터 생성
        std::string workspaceUuid = drogon::utils::getUuid();
        std::string timestamp     = utils::getCurrentTimestamp();
        std::string timestampStr  = timestamp.substr(0, 19);  // YYYY-MM-DDTHH:MM:SS
        std::replace(timestampStr.begin(), timestampStr.end(), 'T', '_');
        std::replace(timestampStr.begin(), timestampStr.end(), ':', '-');
        std::string workspaceName = deviceId + "_" + timestampStr;

        services::WorkspaceMetadata metadata;
        metadata.uuid        = workspaceUuid;
        metadata.target      = deviceId;
        metadata.name        = workspaceName;
        metadata.description = "backup from " + deviceId;
        metadata.createdAt   = timestamp;
        metadata.updatedAt   = timestamp;

        // 10. workspace로 import
        auto importResult =
                services::WorkspaceService::importWorkspace(tempFile, metadata, deviceId);

        // 11. 임시 파일 삭제
        fs::remove(tempFile);

        if (!importResult.success) {
            return services::ServiceResult::createError("Failed to import workspace: " +
                                                        importResult.errorMessage);
        }

        LOG_INFO << "원격 백업 완료: " << deviceId << " (Workspace: " << workspaceUuid << ")";
        return importResult;

    } catch (const std::exception &e) {
        LOG_ERROR << "원격 백업 다운로드 중 오류: " << e.what();
        return services::ServiceResult::createError("원격 백업 다운로드 실패: " +
                                                    std::string(e.what()));
    }
}

services::ServiceResult FileTransferService::applyWorkspace(const std::string &workspaceId,
                                                            const std::string &deviceId,
                                                            const std::string &password)
{
    try {
        LOG_INFO << "워크스페이스 적용 시작: workspaceId=" << workspaceId
                             << ", deviceId=" << deviceId;

        // 1. Device 정보 조회
        auto deviceResult = services::DeviceService::readDevice(deviceId);
        if (!deviceResult.success) {
            return services::ServiceResult::createError("Device not found: " + deviceId);
        }

        // 2. Device metadata에서 SFTP 및 API 정보 추출
        utils::ConnectionParams connParams;
        auto extractResult =
                utils::DeviceMetadataHelper::extractConnectionParamsFromJson(deviceResult.data,
                                                                              connParams);
        if (!extractResult.success) {
            return services::ServiceResult::createError("Invalid device metadata: " +
                                                        extractResult.errorMessage);
        }

        std::string sftpHost = connParams.sftpHost;
        std::string sftpUser = connParams.sftpUser;
        std::string api      = connParams.apiUrl;

        LOG_INFO << "Device info: api=" << api << ", sftpHost=" << sftpHost
                             << ", sftpUser=" << sftpUser;

        // 3. 임시 디렉토리에 workspace export
        std::string tempDir = "/tmp/apply_";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string tempFile = tempDir + drogon::utils::getUuid() + ".tar.gz";

        // 4. Workspace export
        auto exportResult =
                services::WorkspaceService::exportWorkspace(workspaceId, tempFile, deviceId);

        if (!exportResult.success) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("Failed to export workspace: " +
                                                        exportResult.errorMessage);
        }

        // 5. SFTP로 원격 서버에 파일 업로드
        SFTPConfig sftpConfig = utils::DeviceMetadataHelper::createSFTPConfig(connParams);
        SFTPClient sftpClient(sftpConfig);

        if (!sftpClient.connect()) {
            fs::remove(tempFile);
            return services::ServiceResult::createError(sftpClient.getLastError());
        }

        std::string remotePath = "/home/" + sftpUser + "/input.tgz";
        LOG_INFO << "SFTP 파일 업로드 시작: " << tempFile << " -> " << remotePath;

        if (!sftpClient.uploadFile(tempFile, remotePath)) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("SFTP 파일 업로드 실패: " +
                                                        sftpClient.getLastError());
        }

        LOG_INFO << "SFTP 파일 업로드 성공";

        // 6. 임시 파일 삭제
        fs::remove(tempFile);

        // 7. Workspace Extract API 호출하여 압축 해제
        auto apiResult = extractWorkspace(sftpUser, api);
        if (!apiResult.success) {
            std::string message = apiResult.errorMessage;
            if (!apiResult.remoteMessage.empty()) {
                message += ": " + apiResult.remoteMessage;
            }
            return services::ServiceResult::createError(message);
        }

        LOG_INFO << "워크스페이스 압축 해제 성공";

        services::ServiceResult result;
        result.success             = true;
        result.data["status"]      = "success";
        result.data["workspaceId"] = workspaceId;
        result.data["target"]      = deviceId;

        LOG_INFO << "워크스페이스 적용 완료: " << deviceId << " (Workspace: " << workspaceId
                             << ")";
        return result;

    } catch (const std::exception &e) {
        LOG_ERROR << "워크스페이스 적용 중 오류: " << e.what();
        return services::ServiceResult::createError("워크스페이스 적용 실패: " +
                                                    std::string(e.what()));
    }
}

RemoteApiResult FileTransferService::compressWorkspace(const std::string &user,
                                                       const std::string &api)
{
    try {
        LOG_INFO << "Workspace 압축 API 호출 준비: user=" << user;

        // Workspace Compress API 호출
        auto client = drogon::HttpClient::newHttpClient(api);
        auto req    = drogon::HttpRequest::newHttpJsonRequest(Json::Value());
        req->setMethod(drogon::Post);
        req->setPath("/api/workspace/compress");

        Json::Value body;
        body["user"] = user;
        req->setBody(body.toStyledString());
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::promise<RemoteApiResult> promise;
        auto                          future = promise.get_future();

        client->sendRequest(
                req, [&promise](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                    if (result != drogon::ReqResult::Ok) {
                        std::string error = "Workspace 압축 API 호출 실패: 네트워크 오류";
                        LOG_ERROR << error;
                        promise.set_value({false, error, ""});
                        return;
                    }

                    int statusCode = response->getStatusCode();
                    if (statusCode != drogon::k200OK) {
                        std::string error;
                        std::string remoteMessage;

                        if (statusCode == drogon::k401Unauthorized) {
                            error = "원격 서버 인증 실패";
                        } else {
                            error = "원격 서버 압축 실패 (HTTP " + std::to_string(statusCode) + ")";

                            // Try to extract message from response body
                            auto jsonResponse = response->getJsonObject();
                            if (jsonResponse && jsonResponse->isMember("message")) {
                                remoteMessage = (*jsonResponse)["message"].asString();
                            } else if (jsonResponse && jsonResponse->isMember("error")) {
                                remoteMessage = (*jsonResponse)["error"].asString();
                            }
                        }
                        LOG_ERROR << error << (remoteMessage.empty() ? "" : " - " + remoteMessage);
                        promise.set_value({false, error, remoteMessage});
                        return;
                    }

                    auto jsonResponse = response->getJsonObject();
                    if (!jsonResponse || !jsonResponse->isMember("success")) {
                        std::string error = "Workspace 압축 API 응답 형식 오류";
                        LOG_ERROR << error;
                        promise.set_value({false, error, ""});
                        return;
                    }

                    bool success = (*jsonResponse)["success"].asBool();
                    if (success) {
                        std::string message = "원격 서버 압축 성공";
                        if (jsonResponse->isMember("message")) {
                            message = (*jsonResponse)["message"].asString();
                        }
                        LOG_INFO << message;
                        promise.set_value({true, "", message});
                    } else {
                        std::string error = "원격 서버 압축 실패";
                        std::string remoteMessage;
                        if (jsonResponse->isMember("message")) {
                            remoteMessage = (*jsonResponse)["message"].asString();
                        } else if (jsonResponse->isMember("error")) {
                            remoteMessage = (*jsonResponse)["error"].asString();
                        }
                        LOG_ERROR << error << (remoteMessage.empty() ? "" : " - " + remoteMessage);
                        promise.set_value({false, error, remoteMessage});
                    }
                });

        return future.get();

    } catch (const std::exception &e) {
        std::string error = "Workspace 압축 중 오류: " + std::string(e.what());
        LOG_ERROR << error;
        return {false, error, ""};
    }
}

RemoteApiResult FileTransferService::extractWorkspace(const std::string &user,
                                                      const std::string &api)
{
    try {
        LOG_INFO << "Workspace 압축 해제 API 호출 준비: user=" << user;

        // Workspace Extract API 호출
        auto client = drogon::HttpClient::newHttpClient(api);
        auto req    = drogon::HttpRequest::newHttpJsonRequest(Json::Value());
        req->setMethod(drogon::Post);
        req->setPath("/api/workspace/extract");

        Json::Value body;
        body["user"] = user;
        req->setBody(body.toStyledString());
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::promise<RemoteApiResult> promise;
        auto                          future = promise.get_future();

        client->sendRequest(
                req, [&promise](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                    if (result != drogon::ReqResult::Ok) {
                        std::string error = "Workspace 압축 해제 API 호출 실패: 네트워크 오류";
                        LOG_ERROR << error;
                        promise.set_value({false, error, ""});
                        return;
                    }

                    int statusCode = response->getStatusCode();
                    if (statusCode != drogon::k200OK) {
                        std::string error;
                        std::string remoteMessage;

                        if (statusCode == drogon::k401Unauthorized) {
                            error = "원격 서버 인증 실패";
                        } else {
                            error = "원격 서버 압축 해제 실패 (HTTP " + std::to_string(statusCode) + ")";

                            // Try to extract message from response body
                            auto jsonResponse = response->getJsonObject();
                            if (jsonResponse && jsonResponse->isMember("message")) {
                                remoteMessage = (*jsonResponse)["message"].asString();
                            } else if (jsonResponse && jsonResponse->isMember("error")) {
                                remoteMessage = (*jsonResponse)["error"].asString();
                            }
                        }
                        LOG_ERROR << error << (remoteMessage.empty() ? "" : " - " + remoteMessage);
                        promise.set_value({false, error, remoteMessage});
                        return;
                    }

                    auto jsonResponse = response->getJsonObject();
                    if (!jsonResponse || !jsonResponse->isMember("success")) {
                        std::string error = "Workspace 압축 해제 API 응답 형식 오류";
                        LOG_ERROR << error;
                        promise.set_value({false, error, ""});
                        return;
                    }

                    bool success = (*jsonResponse)["success"].asBool();
                    if (success) {
                        std::string message = "원격 서버 압축 해제 성공";
                        if (jsonResponse->isMember("message")) {
                            message = (*jsonResponse)["message"].asString();
                        }
                        LOG_INFO << message;
                        promise.set_value({true, "", message});
                    } else {
                        std::string error = "원격 서버 압축 해제 실패";
                        std::string remoteMessage;
                        if (jsonResponse->isMember("message")) {
                            remoteMessage = (*jsonResponse)["message"].asString();
                        } else if (jsonResponse->isMember("error")) {
                            remoteMessage = (*jsonResponse)["error"].asString();
                        }
                        LOG_ERROR << error << (remoteMessage.empty() ? "" : " - " + remoteMessage);
                        promise.set_value({false, error, remoteMessage});
                    }
                });

        return future.get();

    } catch (const std::exception &e) {
        std::string error = "Workspace 압축 해제 중 오류: " + std::string(e.what());
        LOG_ERROR << error;
        return {false, error, ""};
    }
}

// ============================================================================
// 비밀번호 관리 메서드
// ============================================================================

std::string FileTransferService::getPasswordFilePath()
{
    std::string baseDir = utils::config::getBaseDir();
    return baseDir + "/.password";
}

void FileTransferService::ensurePasswordFileExists()
{
    std::string passwordFile = getPasswordFilePath();

    if (fs::exists(passwordFile)) {
        return;  // 이미 존재하면 아무것도 하지 않음
    }

    LOG_INFO << "초기 비밀번호 파일 생성 중...";

    // 기본 비밀번호 "0000"의 해시 생성
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    int ret = bcrypt_gensalt(12, salt);
    if (ret != 0) {
        LOG_ERROR << "비밀번호 salt 생성 실패";
        return;
    }

    ret = bcrypt_hashpw("0000", salt, hash);
    if (ret != 0) {
        LOG_ERROR << "비밀번호 해시 생성 실패";
        return;
    }

    // 해시를 파일에 저장
    if (writePasswordHash(std::string(hash))) {
        LOG_INFO << "초기 비밀번호(0000) 설정 완료";
    } else {
        LOG_ERROR << "초기 비밀번호 파일 쓰기 실패";
    }
}

std::string FileTransferService::readPasswordHash()
{
    ensurePasswordFileExists();  // 파일이 없으면 생성

    std::string   passwordFile = getPasswordFilePath();
    std::ifstream file(passwordFile);

    if (!file.is_open()) {
        LOG_ERROR << "비밀번호 파일 읽기 실패: " << passwordFile;
        return "";
    }

    std::string hash;
    std::getline(file, hash);
    file.close();

    return hash;
}

bool FileTransferService::writePasswordHash(const std::string &hash)
{
    std::string passwordFile = getPasswordFilePath();

    // storage 디렉토리가 없으면 생성
    std::string baseDir = utils::config::getBaseDir();
    if (!fs::exists(baseDir)) {
        fs::create_directories(baseDir);
    }

    std::ofstream file(passwordFile);
    if (!file.is_open()) {
        LOG_ERROR << "비밀번호 파일 쓰기 실패: " << passwordFile;
        return false;
    }

    file << hash;
    file.close();

    return true;
}

bool FileTransferService::verifyPassword(const std::string &password)
{
    if (password.empty()) {
        LOG_WARN << "빈 비밀번호 검증 시도";
        return false;
    }

    std::string storedHash = readPasswordHash();
    if (storedHash.empty()) {
        LOG_ERROR << "저장된 비밀번호 해시가 없습니다";
        return false;
    }

    int ret = bcrypt_checkpw(password.c_str(), storedHash.c_str());

    if (ret == -1) {
        LOG_ERROR << "비밀번호 검증 중 오류 발생";
        return false;
    }

    if (ret == 0) {
        LOG_INFO << "비밀번호 검증 성공";
        return true;
    } else {
        LOG_WARN << "비밀번호 불일치";
        return false;
    }
}

services::ServiceResult FileTransferService::changePassword(const std::string &oldPassword,
                                                            const std::string &newPassword)
{
    // 1. 기존 비밀번호 검증
    if (!verifyPassword(oldPassword)) {
        LOG_WARN << "비밀번호 변경 실패: 기존 비밀번호 불일치";
        return services::ServiceResult::createError("기존 비밀번호가 일치하지 않습니다");
    }

    // 2. 새 비밀번호 유효성 검사
    if (newPassword.empty()) {
        LOG_WARN << "비밀번호 변경 실패: 새 비밀번호가 비어있음";
        return services::ServiceResult::createError("새 비밀번호는 비어있을 수 없습니다");
    }

    // 3. 새 비밀번호 해시 생성
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    int ret = bcrypt_gensalt(12, salt);
    if (ret != 0) {
        LOG_ERROR << "비밀번호 salt 생성 실패";
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    ret = bcrypt_hashpw(newPassword.c_str(), salt, hash);
    if (ret != 0) {
        LOG_ERROR << "비밀번호 해시 생성 실패";
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    // 4. 새 해시 저장
    if (!writePasswordHash(std::string(hash))) {
        LOG_ERROR << "비밀번호 파일 저장 실패";
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    LOG_INFO << "비밀번호 변경 완료";
    return services::ServiceResult::createSuccess("비밀번호가 성공적으로 변경되었습니다");
}
