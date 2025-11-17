#include "FileTransferService.h"

#include <algorithm>
#include <ctime>
#include <drogon/HttpClient.h>
#include <drogon/utils/Utilities.h>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/TimeUtils.h"
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

services::ServiceResult FileTransferService::backupFromRemote(const std::string &deviceId,
                                                              const std::string &password,
                                                              const std::string &remotePath)
{
    try {
        utils::logging::info("원격 백업 다운로드 시작: deviceId=" + deviceId +
                             ", remote=" + remotePath);

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

        // 5. 원격 파일 경로 구성
        std::string fullRemotePath = "/home/" + sftpUser + "/" + remotePath;
        utils::logging::info("원격 파일 전체 경로: " + fullRemotePath);

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

        // 1. Workspace 정보 조회
        auto workspaceResult = services::WorkspaceService::readWorkspace(workspaceId, deviceId);
        if (!workspaceResult.success) {
            return services::ServiceResult::createError("Workspace not found: " + workspaceId);
        }

        // // 2. Workspace의 target (deviceId) 추출
        // std::string deviceId = workspaceResult.data["target"].asString();
        // if (deviceId.empty()) {
        //     return services::ServiceResult::createError("Workspace has no target device");
        // }

        utils::logging::info("Workspace target device: " + deviceId);

        // 3. Device 정보 조회
        auto deviceResult = services::DeviceService::readDevice(deviceId);
        if (!deviceResult.success) {
            return services::ServiceResult::createError("Device not found: " + deviceId);
        }

        // 4. Device metadata에서 SFTP 및 API 정보 추출
        std::string api          = deviceResult.data["api"].asString();
        std::string sftpHost     = deviceResult.data["sftpHost"].asString();
        int         sftpPort     = deviceResult.data["sftpPort"].asInt();
        std::string sftpPassword = deviceResult.data["sftpPassword"].asString();
        std::string sftpUser     = deviceResult.data["sftpUser"].asString();

        utils::logging::info("Device info: api=" + api + ", sftpHost=" + sftpHost +
                             ", sftpUser=" + sftpUser);

        // 5. 임시 디렉토리에 workspace export
        std::string tempDir = "/tmp/apply_";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string tempFile = tempDir + drogon::utils::getUuid() + ".tar.gz";

        // 6. Workspace export
        auto exportResult =
                services::WorkspaceService::exportWorkspace(workspaceId, tempFile, deviceId);

        if (!exportResult.success) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("Failed to export workspace: " +
                                                        exportResult.errorMessage);
        }

        // 7. SFTP로 원격 서버에 파일 업로드
        SFTPConfig sftpConfig(sftpHost, sftpPort, sftpUser, sftpPassword);
        SFTPClient sftpClient(sftpConfig);

        if (!sftpClient.connect()) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("SFTP 연결 실패: " +
                                                        sftpClient.getLastError());
        }

        std::string remotePath = "/home/" + sftpUser + "/workspace.tgz";
        utils::logging::info("SFTP 파일 업로드 시작: " + tempFile + " -> " + remotePath);

        if (!sftpClient.uploadFile(tempFile, remotePath)) {
            fs::remove(tempFile);
            return services::ServiceResult::createError("SFTP 파일 업로드 실패: " +
                                                        sftpClient.getLastError());
        }

        utils::logging::info("SFTP 파일 업로드 성공");

        // 8. 임시 파일 삭제
        fs::remove(tempFile);

        // 9. Workspace Extract API 호출하여 압축 해제
        auto [success, errorMsg] = extractWorkspace(sftpUser, password, api);
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
                                                                   const std::string &password,
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
        body["user"]     = user;
        body["password"] = password;
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
