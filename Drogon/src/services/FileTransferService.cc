#include "FileTransferService.h"

#include <ctime>
#include <drogon/HttpClient.h>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

services::ServiceResult FileTransferService::backupFromRemote(const SFTPConfig  &sftpConfig,
                                                               const std::string &user,
                                                               const std::string &remotePath,
                                                               const std::string &localPath,
                                                               const std::string &api)
{
    std::string downloadPath;

    try {
        utils::logging::info("원격 백업 다운로드 시작: user=" + user + ", remote=" + remotePath);

        // 1. 먼저 원격 서버의 workspace compress API 호출
        std::string apiUrl = api.empty() ? ("https://" + sftpConfig.host) : api;
        utils::logging::info("원격 서버 압축 API 호출: " + apiUrl + "/api/workspace/compress");

        auto client = drogon::HttpClient::newHttpClient(apiUrl);
        auto req    = drogon::HttpRequest::newHttpJsonRequest(Json::Value());
        req->setMethod(drogon::Post);
        req->setPath("/api/workspace/compress");

        Json::Value body;
        body["user"] = user;
        req->setBody(body.toStyledString());
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::promise<bool> compressPromise;
        auto               compressFuture = compressPromise.get_future();

        client->sendRequest(req, [&compressPromise](drogon::ReqResult                result,
                                                     const drogon::HttpResponsePtr &response) {
            if (result != drogon::ReqResult::Ok) {
                utils::logging::error("원격 압축 API 호출 실패: 네트워크 오류");
                compressPromise.set_value(false);
                return;
            }

            if (response->getStatusCode() != drogon::k200OK) {
                utils::logging::error("원격 압축 API 실패: HTTP " +
                                      std::to_string(response->getStatusCode()));
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

        // 2. SFTP 연결
        SFTPClient sftpClient(sftpConfig);
        if (!sftpClient.connect()) {
            return services::ServiceResult::createError("SFTP 연결 실패: " + sftpClient.getLastError());
        }

        // 3. 원격 파일 경로 구성
        std::string fullRemotePath;
        if (remotePath.find('/') == std::string::npos) {
            // remotePath가 단순 이름이면 /home/{user}/ 경로 추가
            // "workspace" -> "/home/{user}/workspace.tgz"
            if (remotePath == "workspace") {
                fullRemotePath = "/home/" + user + "/workspace.tgz";
            } else if (remotePath.find('.') == std::string::npos) {
                // 확장자가 없으면 .tgz 추가
                fullRemotePath = "/home/" + user + "/" + remotePath + ".tgz";
            } else {
                fullRemotePath = "/home/" + user + "/" + remotePath;
            }
        } else {
            // 이미 경로가 포함되어 있으면 그대로 사용
            fullRemotePath = remotePath;
        }

        utils::logging::info("원격 파일 전체 경로: " + fullRemotePath);

        // 4. 다운로드 경로 결정
        if (localPath.empty()) {
            // localPath가 없으면 임시 파일로 다운로드
            auto               now = std::time(nullptr);
            auto               tm  = *std::localtime(&now);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
            downloadPath = "/tmp/backup_" + oss.str() + ".tgz";
        } else {
            // localPath가 디렉토리인지 확인
            bool isDirectory = false;
            if (localPath.back() == '/') {
                // '/'로 끝나면 디렉토리
                isDirectory = true;
            } else if (fs::exists(localPath) && fs::is_directory(localPath)) {
                // 이미 존재하는 디렉토리
                isDirectory = true;
            } else if (localPath.find('.') == std::string::npos ||
                       localPath.find('.') < localPath.find_last_of('/')) {
                // 확장자가 없거나, 마지막 '/' 이후에 '.'이 없으면 디렉토리로 간주
                isDirectory = true;
            }

            if (isDirectory) {
                // 디렉토리이면 파일명 추가
                downloadPath = localPath;
                if (downloadPath.back() != '/') {
                    downloadPath += "/";
                }
                downloadPath += "workspace.tgz";
            } else {
                downloadPath = localPath;
            }
        }

        // 5. localPath의 디렉토리가 존재하는지 확인하고 생성
        if (!localPath.empty()) {
            fs::path dirPath = fs::path(downloadPath).parent_path();
            if (!dirPath.empty() && !fs::exists(dirPath)) {
                try {
                    fs::create_directories(dirPath);
                    utils::logging::info("로컬 디렉토리 생성: " + dirPath.string());
                } catch (const std::exception &e) {
                    return services::ServiceResult::createError(
                            "로컬 디렉토리 생성 실패: " + std::string(e.what()));
                }
            }
        }

        // 6. SFTP로 파일 다운로드
        utils::logging::info("파일 다운로드 시작: " + fullRemotePath + " -> " + downloadPath);
        if (!sftpClient.downloadFile(fullRemotePath, downloadPath)) {
            return services::ServiceResult::createError("파일 다운로드 실패: " +
                                                        sftpClient.getLastError());
        }

        utils::logging::info("파일 다운로드 성공: " + downloadPath);

        // 7. 다운로드한 파일이 존재하는지 확인
        if (!fs::exists(downloadPath) || !fs::is_regular_file(downloadPath)) {
            return services::ServiceResult::createError("다운로드한 파일이 존재하지 않음: " +
                                                        downloadPath);
        }

        services::ServiceResult result;
        result.success                = true;
        result.data["downloadPath"]   = downloadPath;
        result.data["remotePath"]     = fullRemotePath;
        result.data["status"]         = "downloaded";

        if (!localPath.empty()) {
            result.data["localPath"] = localPath;
        }

        utils::logging::info("원격 백업 다운로드 완료");
        return result;

    } catch (const std::exception &e) {
        utils::logging::error("원격 백업 다운로드 중 오류: " + std::string(e.what()));
        return services::ServiceResult::createError("원격 백업 다운로드 실패: " +
                                                    std::string(e.what()));
    }
}

services::ServiceResult FileTransferService::applyWorkspace(const std::string &uploadedFilePath,
                                                            const std::string &user,
                                                            const std::string &password,
                                                            const std::string &sftpHost,
                                                            int                sftpPort,
                                                            const std::string &api)
{
    try {
        utils::logging::info("워크스페이스 복원 시작 (HTTP 업로드): user=" + user +
                             ", file=" + uploadedFilePath);

        // 업로드된 파일이 존재하는지 확인
        if (!fs::exists(uploadedFilePath) || !fs::is_regular_file(uploadedFilePath)) {
            return services::ServiceResult::createError("업로드된 파일이 존재하지 않음: " +
                                                        uploadedFilePath);
        }

        // Workspace Extract API 호출하여 압축 해제
        auto [success, errorMsg] = extractWorkspace(user, password, uploadedFilePath, api);
        if (!success) {
            return services::ServiceResult::createError(errorMsg);
        }

        utils::logging::info("워크스페이스 압축 해제 성공");

        services::ServiceResult result;
        result.success        = true;
        result.data["status"] = "success";

        utils::logging::info("워크스페이스 복원 완료");
        return result;

    } catch (const std::exception &e) {
        utils::logging::error("워크스페이스 복원 중 오류: " + std::string(e.what()));
        return services::ServiceResult::createError("워크스페이스 복원 실패: " +
                                                    std::string(e.what()));
    }
}

std::pair<bool, std::string> FileTransferService::extractWorkspace(const std::string &user,
                                                                    const std::string &password,
                                                                    const std::string &archivePath,
                                                                    const std::string &api)
{
    try {
        utils::logging::info("Workspace 압축 해제 준비: user=" + user + ", archive=" + archivePath);

        // 1. 다운로드한 압축 파일을 고정 경로로 복사
        std::string targetPath = "/home/" + user + "/workspace.tgz";
        utils::logging::info("압축 파일 복사: " + archivePath + " -> " + targetPath);

        try {
            fs::copy_file(archivePath, targetPath, fs::copy_options::overwrite_existing);
        } catch (const std::exception &e) {
            std::string error = "압축 파일 복사 실패: " + std::string(e.what());
            utils::logging::error(error);
            return {false, error};
        }

        // 2. Workspace Extract API 호출
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

        client->sendRequest(req, [&promise](drogon::ReqResult                result,
                                            const drogon::HttpResponsePtr &response) {
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
                    error = "Workspace 압축 해제 API 실패: HTTP " + std::to_string(statusCode);
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
