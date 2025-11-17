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
    if (!jsonBody->isMember("workspaceId") || (*jsonBody)["workspaceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: workspaceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("deviceId") || (*jsonBody)["deviceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: deviceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("password") || (*jsonBody)["password"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: password";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string workspaceId = (*jsonBody)["workspaceId"].asString();
    std::string deviceId    = (*jsonBody)["deviceId"].asString();
    std::string password    = (*jsonBody)["password"].asString();

    auto task = [workspaceId, deviceId, password, callback]() {
        FileTransferService service;
        auto                result = service.applyWorkspace(workspaceId, deviceId, password);

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
    if (!jsonBody->isMember("deviceId") || (*jsonBody)["deviceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: deviceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("password") || (*jsonBody)["password"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["error"]   = "Missing required field: password";
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

    std::string deviceId   = (*jsonBody)["deviceId"].asString();
    std::string password   = (*jsonBody)["password"].asString();
    std::string remotePath = (*jsonBody)["remotePath"].asString();

    auto task = [deviceId, password, remotePath, callback]() {
        FileTransferService service;
        auto                result = service.backupFromRemote(deviceId, password, remotePath);

        if (!result.success) {
            Json::Value error;
            error["success"] = false;
            error["error"]   = result.errorMessage;
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value response;
        response["success"] = true;
        response["data"]    = result.data;

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK);
        callback(resp);
    };

    std::thread(task).detach();
}
