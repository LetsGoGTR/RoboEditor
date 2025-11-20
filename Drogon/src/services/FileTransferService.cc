#include "FileTransferService.h"

#include <algorithm>
#include <extbcrypt.h>
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
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

services::ServiceResult FileTransferService::backupFromRemote(const std::string &deviceId)
{
    try {
        utils::logging::info("원격 백업 다운로드 시작: deviceId=" + deviceId);

        // 1. Device 정보 조회
        auto deviceResult = services::DeviceService::readDevice(deviceId);
        if (!deviceResult.success) {
            return services::ServiceResult::createError("Device not found: " + deviceId);
        }

        // 2. Device metadata에서 SFTP 및 API 정보 추출
        std::string api          = deviceResult.data["api"].asString();
        std::string sftpHost     = deviceResult.data["sftpHost"].asString();
        int         sftpPort     = deviceResult.data["sftpPort"].asInt();
        std::string sftpPassword = deviceResult.data["sftpPassword"].asString();
        std::string sftpUser     = deviceResult.data["sftpUser"].asString();

        utils::logging::info("Device info: api=" + api + ", sftpHost=" + sftpHost +
                             ", sftpUser=" + sftpUser);

        // 3. 원격 서버의 workspace compress API 호출
        std::string apiUrl = api.empty() ? ("https://" + sftpHost) : api;
        utils::logging::info("원격 서버 압축 API 호출: " + apiUrl + "/api/workspace/compress");

        auto client = drogon::HttpClient::newHttpClient(apiUrl);
        auto req    = drogon::HttpRequest::newHttpJsonRequest(Json::Value());
        req->setMethod(drogon::Post);
        req->setPath("/api/workspace/compress");

        Json::Value body;
        body["user"] = sftpUser;
        req->setBody(body.toStyledString());
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::promise<bool> compressPromise;
        auto               compressFuture = compressPromise.get_future();

        client->sendRequest(req,
                            [&compressPromise](drogon::ReqResult              result,
                                               const drogon::HttpResponsePtr &response) {
                                if (result != drogon::ReqResult::Ok) {
                                    utils::logging::error("원격 압축 API 호출 실패: 네트워크 오류");
                                    compressPromise.set_value(false);
                                    return;
                                }

                                if (response->getStatusCode() != drogon::k200OK) {
                                    std::string errorMsg =
                                            "원격 압축 API 실패: HTTP " +
                                            std::to_string(response->getStatusCode());

                                    auto jsonResponse = response->getJsonObject();
                                    if (jsonResponse && jsonResponse->isMember("error")) {
                                        errorMsg += " - " + (*jsonResponse)["error"].asString();
                                    } else {
                                        auto body = response->getBody();
                                        if (!body.empty() && body.size() < 200) {
                                            errorMsg += " - " + std::string(body);
                                        }
                                    }

                                    utils::logging::error(errorMsg);
                                    compressPromise.set_value(false);
                                    return;
                                }

                                auto jsonResponse = response->getJsonObject();
                                if (!jsonResponse || !jsonResponse->isMember("success")) {
                                    utils::logging::error("원격 압축 API 응답 형식 오류");
                                    compressPromise.set_value(false);
                                    return;
                                }

                                bool success = (*jsonResponse)["success"].asBool();
                                if (success) {
                                    utils::logging::info("원격 서버 압축 성공");
                                } else {
                                    utils::logging::error("원격 서버 압축 실패");
                                }
                                compressPromise.set_value(success);
                            });

        bool compressSuccess = compressFuture.get();
        if (!compressSuccess) {
            return services::ServiceResult::createError("원격 서버 압축 실패");
        }

        // 4. SFTP 연결
        SFTPConfig sftpConfig(sftpHost, sftpPort, sftpUser, sftpPassword);
        SFTPClient sftpClient(sftpConfig);
        if (!sftpClient.connect()) {
            return services::ServiceResult::createError("SFTP 연결 실패: " +
                                                        sftpClient.getLastError());
        }

        // 5. 원격 파일 경로 구성 (output.tgz 고정)
        std::string fullRemotePath = "/home/" + sftpUser + "/output.tgz";
        utils::logging::info("원격 파일 다운로드: " + fullRemotePath);

        // 6. 임시 디렉토리에 다운로드
        std::string tempDir = "/tmp/backup_";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string tempFile = tempDir + drogon::utils::getUuid() + ".tar.gz";

        // 7. SFTP로 파일 다운로드
        utils::logging::info("파일 다운로드 시작: " + fullRemotePath + " -> " + tempFile);
        if (!sftpClient.downloadFile(fullRemotePath, tempFile)) {
            return services::ServiceResult::createError("파일 다운로드 실패: " +
                                                        sftpClient.getLastError());
        }

        utils::logging::info("파일 다운로드 성공: " + tempFile);

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

        utils::logging::info("원격 백업 완료: " + deviceId + " (Workspace: " + workspaceUuid + ")");
        return importResult;

    } catch (const std::exception &e) {
        utils::logging::error("원격 백업 다운로드 중 오류: " + std::string(e.what()));
        return services::ServiceResult::createError("원격 백업 다운로드 실패: " +
                                                    std::string(e.what()));
    }
}

services::ServiceResult FileTransferService::applyWorkspace(const std::string &workspaceId,
                                                            const std::string &deviceId,
                                                            const std::string &password)
{
    try {
        utils::logging::info("워크스페이스 적용 시작: workspaceId=" + workspaceId +
                             ", deviceId=" + deviceId);

        // 1. Device 정보 조회
        auto deviceResult = services::DeviceService::readDevice(deviceId);
        if (!deviceResult.success) {
            return services::ServiceResult::createError("Device not found: " + deviceId);
        }

        // 2. Device metadata에서 SFTP 및 API 정보 추출
        std::string api          = deviceResult.data["api"].asString();
        std::string sftpHost     = deviceResult.data["sftpHost"].asString();
        int         sftpPort     = deviceResult.data["sftpPort"].asInt();
        std::string sftpPassword = deviceResult.data["sftpPassword"].asString();
        std::string sftpUser     = deviceResult.data["sftpUser"].asString();

        utils::logging::info("Device info: api=" + api + ", sftpHost=" + sftpHost +
                             ", sftpUser=" + sftpUser);

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
        SFTPConfig sftpConfig(sftpHost, sftpPort, sftpUser, sftpPassword);
        SFTPClient sftpClient(sftpConfig);

        if (!sftpClient.connect()) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("SFTP 연결 실패: " +
                                                        sftpClient.getLastError());
        }

        std::string remotePath = "/home/" + sftpUser + "/input.tgz";
        utils::logging::info("SFTP 파일 업로드 시작: " + tempFile + " -> " + remotePath);

        if (!sftpClient.uploadFile(tempFile, remotePath)) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("SFTP 파일 업로드 실패: " +
                                                        sftpClient.getLastError());
        }

        utils::logging::info("SFTP 파일 업로드 성공");

        // 6. 임시 파일 삭제
        fs::remove(tempFile);

        // 7. Workspace Extract API 호출하여 압축 해제
        auto [success, errorMsg] = extractWorkspace(sftpUser, api);
        if (!success) {
            return services::ServiceResult::createError(errorMsg);
        }

        utils::logging::info("워크스페이스 압축 해제 성공");

        services::ServiceResult result;
        result.success             = true;
        result.data["status"]      = "success";
        result.data["workspaceId"] = workspaceId;
        result.data["target"]      = deviceId;

        utils::logging::info("워크스페이스 적용 완료: " + deviceId + " (Workspace: " + workspaceId +
                             ")");
        return result;

    } catch (const std::exception &e) {
        utils::logging::error("워크스페이스 적용 중 오류: " + std::string(e.what()));
        return services::ServiceResult::createError("워크스페이스 적용 실패: " +
                                                    std::string(e.what()));
    }
}

std::pair<bool, std::string> FileTransferService::extractWorkspace(const std::string &user,
                                                                   const std::string &api)
{
    try {
        utils::logging::info("Workspace 압축 해제 API 호출 준비: user=" + user);

        // Workspace Extract API 호출
        auto client = drogon::HttpClient::newHttpClient(api);
        auto req    = drogon::HttpRequest::newHttpJsonRequest(Json::Value());
        req->setMethod(drogon::Post);
        req->setPath("/api/workspace/extract");

        Json::Value body;
        body["user"] = user;
        req->setBody(body.toStyledString());
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::promise<std::pair<bool, std::string>> promise;
        auto                                       future = promise.get_future();

        client->sendRequest(
                req, [&promise](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                    if (result != drogon::ReqResult::Ok) {
                        std::string error = "Workspace 압축 해제 API 호출 실패: 네트워크 오류";
                        utils::logging::error(error);
                        promise.set_value({false, error});
                        return;
                    }

                    int statusCode = response->getStatusCode();
                    if (statusCode != drogon::k200OK) {
                        std::string error;
                        if (statusCode == drogon::k401Unauthorized) {
                            error = "인증 실패: 비밀번호가 올바르지 않습니다";
                        } else {
                            error = "Workspace 압축 해제 API 실패: HTTP " +
                                    std::to_string(statusCode);
                        }
                        utils::logging::error(error);
                        promise.set_value({false, error});
                        return;
                    }

                    auto jsonResponse = response->getJsonObject();
                    if (!jsonResponse || !jsonResponse->isMember("success")) {
                        std::string error = "Workspace 압축 해제 API 응답 형식 오류";
                        utils::logging::error(error);
                        promise.set_value({false, error});
                        return;
                    }

                    bool success = (*jsonResponse)["success"].asBool();
                    if (success) {
                        utils::logging::info("Workspace 압축 해제 성공");
                        promise.set_value({true, ""});
                    } else {
                        std::string error = "Workspace 압축 해제 실패";
                        if (jsonResponse->isMember("error")) {
                            error = (*jsonResponse)["error"].asString();
                        }
                        utils::logging::error(error);
                        promise.set_value({false, error});
                    }
                });

        return future.get();

    } catch (const std::exception &e) {
        std::string error = "Workspace 압축 해제 중 오류: " + std::string(e.what());
        utils::logging::error(error);
        return {false, error};
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

    utils::logging::info("초기 비밀번호 파일 생성 중...");

    // 기본 비밀번호 "0000"의 해시 생성
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    int ret = bcrypt_gensalt(12, salt);
    if (ret != 0) {
        utils::logging::error("비밀번호 salt 생성 실패");
        return;
    }

    ret = bcrypt_hashpw("0000", salt, hash);
    if (ret != 0) {
        utils::logging::error("비밀번호 해시 생성 실패");
        return;
    }

    // 해시를 파일에 저장
    if (writePasswordHash(std::string(hash))) {
        utils::logging::info("초기 비밀번호(0000) 설정 완료");
    } else {
        utils::logging::error("초기 비밀번호 파일 쓰기 실패");
    }
}

std::string FileTransferService::readPasswordHash()
{
    ensurePasswordFileExists();  // 파일이 없으면 생성

    std::string   passwordFile = getPasswordFilePath();
    std::ifstream file(passwordFile);

    if (!file.is_open()) {
        utils::logging::error("비밀번호 파일 읽기 실패: " + passwordFile);
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
        utils::logging::error("비밀번호 파일 쓰기 실패: " + passwordFile);
        return false;
    }

    file << hash;
    file.close();

    return true;
}

bool FileTransferService::verifyPassword(const std::string &password)
{
    if (password.empty()) {
        utils::logging::warn("빈 비밀번호 검증 시도");
        return false;
    }

    std::string storedHash = readPasswordHash();
    if (storedHash.empty()) {
        utils::logging::error("저장된 비밀번호 해시가 없습니다");
        return false;
    }

    int ret = bcrypt_checkpw(password.c_str(), storedHash.c_str());

    if (ret == -1) {
        utils::logging::error("비밀번호 검증 중 오류 발생");
        return false;
    }

    if (ret == 0) {
        utils::logging::info("비밀번호 검증 성공");
        return true;
    } else {
        utils::logging::warn("비밀번호 불일치");
        return false;
    }
}

services::ServiceResult FileTransferService::changePassword(const std::string &oldPassword,
                                                            const std::string &newPassword)
{
    // 1. 기존 비밀번호 검증
    if (!verifyPassword(oldPassword)) {
        utils::logging::warn("비밀번호 변경 실패: 기존 비밀번호 불일치");
        return services::ServiceResult::createError("기존 비밀번호가 일치하지 않습니다");
    }

    // 2. 새 비밀번호 유효성 검사
    if (newPassword.empty()) {
        utils::logging::warn("비밀번호 변경 실패: 새 비밀번호가 비어있음");
        return services::ServiceResult::createError("새 비밀번호는 비어있을 수 없습니다");
    }

    // 3. 새 비밀번호 해시 생성
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    int ret = bcrypt_gensalt(12, salt);
    if (ret != 0) {
        utils::logging::error("비밀번호 salt 생성 실패");
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    ret = bcrypt_hashpw(newPassword.c_str(), salt, hash);
    if (ret != 0) {
        utils::logging::error("비밀번호 해시 생성 실패");
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    // 4. 새 해시 저장
    if (!writePasswordHash(std::string(hash))) {
        utils::logging::error("비밀번호 파일 저장 실패");
        return services::ServiceResult::createError("비밀번호 변경 중 오류 발생");
    }

    utils::logging::info("비밀번호 변경 완료");
    return services::ServiceResult::createSuccess("비밀번호가 성공적으로 변경되었습니다");
}
