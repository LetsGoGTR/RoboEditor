#include "FileTransferController.h"

#include <ctime>
#include <filesystem>
#include <thread>

#include "../services/FileTransferService.h"
#include "../utils/sftp/SFTPConfig.h"

using namespace api::v1;
using namespace drogon;

void FT::apply(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // multipart/form-data로 파일 업로드 받기
    drogon::MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Invalid multipart/form-data";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto &files = fileUpload.getFiles();
    if (files.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "No file uploaded";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // form-data에서 user와 password 파라미터 가져오기
    auto &parameters = fileUpload.getParameters();

    auto userIt = parameters.find("user");
    if (userIt == parameters.end() || userIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: user";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto passwordIt = parameters.find("password");
    if (passwordIt == parameters.end() || passwordIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: password";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto sftpPasswordIt = parameters.find("sftpPassword");
    if (sftpPasswordIt == parameters.end() || sftpPasswordIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: sftpPassword";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto sftpHostIt = parameters.find("sftpHost");
    if (sftpHostIt == parameters.end() || sftpHostIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: sftpHost";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto sftpPortIt = parameters.find("sftpPort");
    if (sftpPortIt == parameters.end() || sftpPortIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: sftpPort";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto apiIt = parameters.find("api");
    if (apiIt == parameters.end() || apiIt->second.empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: api";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string user         = userIt->second;
    std::string password     = passwordIt->second;
    std::string sftpPassword = sftpPasswordIt->second;
    std::string sftpHost     = sftpHostIt->second;
    int         sftpPort     = std::stoi(sftpPortIt->second);
    std::string api          = apiIt->second;

    // 첫 번째 파일 가져오기
    auto &file = files[0];

    // 업로드된 파일을 임시 경로에 저장
    std::string tmpPath = "/tmp/uploaded_workspace_" + std::to_string(std::time(nullptr)) + ".tgz";
    file.saveAs(tmpPath);

    auto task = [tmpPath, user, password, sftpPassword, sftpHost, sftpPort, api, callback]() {
        FileTransferService service;
        auto                result = service.applyWorkspace(
                tmpPath, user, password, sftpPassword, sftpHost, sftpPort, api);

        // 처리 후 임시 파일 삭제
        try {
            std::filesystem::remove(tmpPath);
        } catch (...) {
        }

        Json::Value response;
        response["success"] = result.success;
        if (result.success) {
            response["data"] = result.data;
        } else {
            response["error"] = result.errorMessage;
        }

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(result.success ? k200OK : k500InternalServerError);
        callback(resp);
    };

    std::thread(task).detach();
}

void FT::backup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Invalid JSON";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 필수 파라미터 검증
    if (!jsonBody->isMember("sftpHost") || (*jsonBody)["sftpHost"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: sftpHost";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (!jsonBody->isMember("sftpPassword")) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: sftpPassword";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (!jsonBody->isMember("user") || (*jsonBody)["user"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: user";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (!jsonBody->isMember("remotePath") || (*jsonBody)["remotePath"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: remotePath";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // sftpPort 기본값 22
    int sftpPort = 22;
    if (jsonBody->isMember("sftpPort")) {
        sftpPort = (*jsonBody)["sftpPort"].asInt();
    }

    SFTPConfig config((*jsonBody)["sftpHost"].asString(),
                      sftpPort,
                      (*jsonBody)["user"].asString(),
                      (*jsonBody)["sftpPassword"].asString());

    std::string user       = (*jsonBody)["user"].asString();
    std::string remotePath = (*jsonBody)["remotePath"].asString();
    std::string api        = jsonBody->isMember("api") ? (*jsonBody)["api"].asString() : "";

    // localPath는 선택적 파라미터
    std::string localPath;
    if (jsonBody->isMember("localPath") && !(*jsonBody)["localPath"].asString().empty()) {
        localPath = (*jsonBody)["localPath"].asString();
    }

    auto task = [config, user, remotePath, localPath, api, callback]() {
        FileTransferService service;
        auto result = service.backupFromRemote(config, user, remotePath, localPath, api);

        if (!result.success) {
            Json::Value error;
            error["success"] = false;
            error["error"]   = result.errorMessage;
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        // localPath가 지정되어 있으면 JSON 응답
        if (!localPath.empty()) {
            Json::Value response;
            response["success"] = true;
            response["data"]    = result.data;

            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k200OK);
            callback(resp);
        } else {
            // localPath가 없으면 HTTP 다운로드
            std::string downloadPath = result.data["downloadPath"].asString();
            auto        resp         = HttpResponse::newFileResponse(
                    downloadPath, "", drogon::CT_APPLICATION_OCTET_STREAM);
            resp->addHeader("Content-Disposition", "attachment; filename=\"workspace.tgz\"");
            callback(resp);
        }
    };

    std::thread(task).detach();
}
