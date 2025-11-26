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
        error["message"] = "Invalid JSON";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 필수 파라미터 검증
    if (!jsonBody->isMember("workspaceId") || (*jsonBody)["workspaceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: workspaceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("deviceId") || (*jsonBody)["deviceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: deviceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("password") || (*jsonBody)["password"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: password";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string workspaceId = (*jsonBody)["workspaceId"].asString();
    std::string deviceId    = (*jsonBody)["deviceId"].asString();
    std::string password    = (*jsonBody)["password"].asString();

    // 비밀번호 검증
    if (!FileTransferService::verifyPassword(password)) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Invalid password";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto task = [workspaceId, deviceId, password, callback]() {
        auto result = FileTransferService::applyWorkspace(workspaceId, deviceId, password);

        Json::Value response;
        response["success"] = result.success;
        if (result.success) {
            response["data"] = result.data;
        } else {
            response["message"] = result.errorMessage;
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
        error["message"] = "Invalid JSON";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 필수 파라미터 검증
    if (!jsonBody->isMember("deviceId") || (*jsonBody)["deviceId"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: deviceId";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string deviceId = (*jsonBody)["deviceId"].asString();

    auto task = [deviceId, callback]() {
        auto result = FileTransferService::backupFromRemote(deviceId);

        Json::Value response;
        response["success"] = result.success;

        if (!result.success) {
            response["message"] = result.errorMessage;
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        response["data"] = result.data;
        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k200OK);
        callback(resp);
    };

    std::thread(task).detach();
}

void FT::changePassword(const HttpRequestPtr                          &req,
                        std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Invalid JSON";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 필수 파라미터 검증
    if (!jsonBody->isMember("oldPassword") || (*jsonBody)["oldPassword"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: oldPassword";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!jsonBody->isMember("newPassword") || (*jsonBody)["newPassword"].asString().empty()) {
        Json::Value error;
        error["success"] = false;
        error["message"] = "Missing required field: newPassword";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string oldPassword = (*jsonBody)["oldPassword"].asString();
    std::string newPassword = (*jsonBody)["newPassword"].asString();

    // 비밀번호 변경
    auto result = FileTransferService::changePassword(oldPassword, newPassword);

    Json::Value response;
    response["success"] = result.success;
    response["message"] = result.errorMessage;  // 성공/실패 모두 message 사용

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(result.success ? k200OK : k401Unauthorized);
    callback(resp);
}
