#include "DeviceController.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"

namespace fs = std::filesystem;

static std::string baseDir = "/tmp/drogon-app/storage/";

// Helper functions
static void sendError(std::function<void(const drogon::HttpResponsePtr &)> &callback,
                      drogon::HttpStatusCode                                status,
                      const std::string                                    &error,
                      const std::string                                    &message = "")
{
    Json::Value json;
    json["error"] = error;
    if (!message.empty())
        json["message"] = message;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(status);
    callback(resp);
}

void api::v1::Device::create(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    if (!json->isMember("id") || !json->isMember("name"))
        return sendError(callback, drogon::k400BadRequest, "Missing required fields: id, name");

    std::string deviceId = (*json)["id"].asString();
    if (deviceId.empty())
        return sendError(callback, drogon::k400BadRequest, "Device ID cannot be empty");

    try {
        services::DeviceMetadata metadata;
        metadata.id   = deviceId;
        metadata.name = (*json)["name"].asString();
        metadata.description =
                json->isMember("description") ? (*json)["description"].asString() : "";
        metadata.ip = json->isMember("ip") ? (*json)["ip"].asString() : "";

        auto result = services::DeviceService::createDevice(baseDir, metadata);

        if (!result.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to create device",
                             result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Device created successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);

        LOG_INFO << "Created device: " << metadata.name << " (ID: " << metadata.id << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Create exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::info(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                           const std::string                                     &target)
{
    try {
        auto result = services::DeviceService::getDeviceInfo(baseDir, target);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Device not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Retrieved device info: " << target;

    } catch (const std::exception &e) {
        LOG_ERROR << "Info exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::update(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &target)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    try {
        // Load existing metadata first
        auto existingResult = services::DeviceService::getDeviceInfo(baseDir, target);
        if (!existingResult.success) {
            return sendError(callback, drogon::k404NotFound, "Device not found", target);
        }

        // Prepare updated metadata, keeping existing values if not provided
        services::DeviceMetadata metadata;
        metadata.name        = json->isMember("name") ? (*json)["name"].asString()
                                                      : existingResult.data["name"].asString();
        metadata.description = json->isMember("description")
                                       ? (*json)["description"].asString()
                                       : existingResult.data["description"].asString();
        metadata.ip          = json->isMember("ip") ? (*json)["ip"].asString()
                                                    : existingResult.data["ip"].asString();

        auto result = services::DeviceService::updateDevice(baseDir, target, metadata);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Device not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Device updated successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Updated device: " << target;

    } catch (const std::exception &e) {
        LOG_ERROR << "Update exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::remove(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &target)
{
    try {
        auto result = services::DeviceService::deleteDevice(baseDir, target);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Device not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Device deleted successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Deleted device: " << target;

    } catch (const std::exception &e) {
        LOG_ERROR << "Delete exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::upload(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &target)
{
}

void api::v1::Device::download(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               const std::string                                     &target)
{
}

void api::v1::Device::restore(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                              const std::string                                     &target)
{
}

void api::v1::Device::backup(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &target)
{
    // Check if device exists
    auto deviceResult = services::DeviceService::getDeviceInfo(baseDir, target);
    if (!deviceResult.success) {
        return sendError(callback, drogon::k404NotFound, "Device not found", target);
    }

    // Get IP from device metadata (default: localhost:80)
    std::string ip = deviceResult.data["ip"].asString();
    if (ip.empty()) {
        ip = "localhost:80";
    }

    // Wrap callback in shared_ptr for lambda capture
    auto callbackPtr = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(
            std::move(callback));

    // Check if robot is running
    auto runningClient = drogon::HttpClient::newHttpClient("http://" + ip);
    auto runningReq    = drogon::HttpRequest::newHttpRequest();
    runningReq->setPath("/api/v1/robot/running");
    runningReq->setMethod(drogon::Get);

    runningClient->sendRequest(
            runningReq,
            [callbackPtr, ip, target, this](drogon::ReqResult              result,
                                            const drogon::HttpResponsePtr &response) {
                if (result != drogon::ReqResult::Ok) {
                    return sendError(*callbackPtr,
                                     drogon::k500InternalServerError,
                                     "Failed to connect to robot",
                                     ip);
                }

                auto json = response->getJsonObject();
                if (!json || !json->isMember("data")) {
                    return sendError(*callbackPtr,
                                     drogon::k500InternalServerError,
                                     "Invalid response from robot");
                }

                bool isRunning = (*json)["data"].asBool();
                if (isRunning) {
                    return sendError(*callbackPtr,
                                     drogon::k409Conflict,
                                     "Robot is running",
                                     "Cannot backup while robot is running");
                }

                // Download archive from robot
                auto exportClient = drogon::HttpClient::newHttpClient("http://" + ip);
                auto exportReq    = drogon::HttpRequest::newHttpRequest();
                exportReq->setPath("/api/v1/robot/export");
                exportReq->setMethod(drogon::Get);

                exportClient->sendRequest(
                        exportReq,
                        [callbackPtr, target, this](drogon::ReqResult              result,
                                                    const drogon::HttpResponsePtr &response) {
                            if (result != drogon::ReqResult::Ok) {
                                return sendError(*callbackPtr,
                                                 drogon::k500InternalServerError,
                                                 "Failed to download archive from robot");
                            }

                            try {
                                // Save archive to temp file
                                std::string tempDir = "/tmp/drogon-backup/";
                                if (!fs::exists(tempDir))
                                    fs::create_directories(tempDir);

                                std::string tempFile =
                                        tempDir + drogon::utils::getUuid() + ".tar.gz";
                                std::ofstream file(tempFile, std::ios::binary);
                                file << response->getBody();
                                file.close();

                                // Generate workspace ID and name
                                std::string workspaceId = drogon::utils::getUuid();
                                std::string timestamp =
                                        services::WorkspaceService::getCurrentTimestamp();
                                std::string timestampStr =
                                        timestamp.substr(0, 19);  // YYYY-MM-DDTHH:MM:SS
                                std::replace(timestampStr.begin(), timestampStr.end(), 'T', '_');
                                std::replace(timestampStr.begin(), timestampStr.end(), ':', '-');
                                std::string workspaceName = target + "_" + timestampStr;

                                // Create workspace metadata
                                services::WorkspaceMetadata metadata;
                                metadata.id          = workspaceId;
                                metadata.target      = target;
                                metadata.name        = workspaceName;
                                metadata.description = "backup from " + target;
                                metadata.createdAt   = timestamp;
                                metadata.updatedAt   = timestamp;

                                // Import workspace into device folder
                                std::string deviceBaseDir = baseDir + target + "/";
                                auto importResult = services::WorkspaceService::importWorkspace(
                                        tempFile, deviceBaseDir, metadata);

                                fs::remove(tempFile);

                                if (!importResult.success) {
                                    return sendError(*callbackPtr,
                                                     drogon::k500InternalServerError,
                                                     "Failed to import workspace",
                                                     importResult.errorMessage);
                                }

                                Json::Value response;
                                response["success"] = true;
                                response["message"] = "Backup completed successfully";
                                response["data"]    = importResult.data;

                                auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
                                resp->setStatusCode(drogon::k200OK);
                                (*callbackPtr)(resp);

                                LOG_INFO << "Backup completed: " << target
                                         << " (Workspace: " << workspaceId << ")";

                            } catch (const std::exception &e) {
                                LOG_ERROR << "Backup exception: " << e.what();
                                sendError(*callbackPtr,
                                          drogon::k500InternalServerError,
                                          "Internal server error",
                                          e.what());
                            }
                        });
            });
}
