#include "DeviceController.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../services/AuthService.h"
#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/TimeUtils.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

static std::string baseDir = drogon::app().getCustomConfig()["storage"]["base_dir"].asString();

using helpers::sendError;
using helpers::sendSuccess;

void api::v1::Device::list(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try {
        auto result = services::DeviceService::listDevices(baseDir);

        if (!result.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to list devices",
                             result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Listed " << result.data["count"].asInt() << " devices";

    } catch (const std::exception &e) {
        LOG_ERROR << "List exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
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
                           const std::string                                     &deviceId)
{
    try {
        auto result = services::DeviceService::readDevice(baseDir, deviceId);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Device not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Retrieved device info: " << deviceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Info exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::update(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &deviceId)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    try {
        // Load existing metadata first
        auto existingResult = services::DeviceService::readDevice(baseDir, deviceId);
        if (!existingResult.success) {
            return sendError(callback, drogon::k404NotFound, "Device not found", deviceId);
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

        auto result = services::DeviceService::updateDevice(baseDir, deviceId, metadata);

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

        LOG_INFO << "Updated device: " << deviceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Update exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::remove(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &deviceId)
{
    try {
        auto result = services::DeviceService::deleteDevice(baseDir, deviceId);

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

        LOG_INFO << "Deleted device: " << deviceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Delete exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Device::apply(const drogon::HttpRequestPtr                          &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                            const std::string                                     &deviceId)
{
    auto json = req->getJsonObject();
    if (!json) {
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");
    }

    if (!json->isMember("password")) {
        return sendError(callback, drogon::k400BadRequest, "Missing password");
    }
    std::string password = (*json)["password"].asString();
    if (!services::AuthService::verifyDevicePassword(password)) {
        return sendError(callback, drogon::k401Unauthorized, "Invalid password");
    }

    // Get workspaceId
    if (!json->isMember("workspaceId")) {
        return sendError(callback, drogon::k400BadRequest, "Missing workspaceId");
    }
    std::string workspaceId = (*json)["workspaceId"].asString();

    // Check if device exists
    auto deviceResult = services::DeviceService::readDevice(baseDir, deviceId);
    if (!deviceResult.success) {
        return sendError(callback, drogon::k404NotFound, "Device not found", deviceId);
    }

    // Get IP from device metadata
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
            [callbackPtr, ip, deviceId, workspaceId, this](
                    drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
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
                                     "Cannot apply while robot is running");
                }

                try {
                    // Export workspace to temp file
                    std::string tempDir = drogon::app().getCustomConfig()["storage"]["temp_apply_dir"].asString();
                    if (!fs::exists(tempDir))
                        fs::create_directories(tempDir);

                    std::string tempFile      = tempDir + drogon::utils::getUuid() + ".tar.gz";
                    std::string deviceBaseDir = baseDir + deviceId + "/";

                    // Export workspace
                    auto exportResult = services::WorkspaceService::exportWorkspace(
                            workspaceId, deviceBaseDir, tempFile);

                    if (!exportResult.success) {
                        fs::remove(tempFile);
                        return sendError(*callbackPtr,
                                         drogon::k404NotFound,
                                         "Failed to export workspace",
                                         exportResult.errorMessage);
                    }

                    // Read file content
                    std::ifstream file(tempFile, std::ios::binary);
                    if (!file.is_open()) {
                        fs::remove(tempFile);
                        return sendError(*callbackPtr,
                                         drogon::k500InternalServerError,
                                         "Failed to read exported file");
                    }

                    std::ostringstream fileBuffer;
                    fileBuffer << file.rdbuf();
                    std::string fileContent = fileBuffer.str();
                    file.close();

                    // Build multipart/form-data body
                    std::string boundary = "----WebKitFormBoundary" + drogon::utils::getUuid();
                    std::ostringstream multipartBody;

                    multipartBody << "--" << boundary << "\r\n";
                    multipartBody << "Content-Disposition: form-data; name=\"file\"; "
                                  << "filename=\"workspace.tar.gz\"\r\n";
                    multipartBody << "Content-Type: application/gzip\r\n\r\n";
                    multipartBody << fileContent;
                    multipartBody << "\r\n--" << boundary << "--\r\n";

                    // Upload to robot
                    auto uploadClient = drogon::HttpClient::newHttpClient("http://" + ip);
                    auto uploadReq    = drogon::HttpRequest::newHttpRequest();
                    uploadReq->setPath("/api/v1/robot/import");
                    uploadReq->setMethod(drogon::Post);
                    uploadReq->setContentTypeString("multipart/form-data; boundary=" + boundary);
                    uploadReq->setBody(multipartBody.str());

                    uploadClient->sendRequest(
                            uploadReq,
                            [callbackPtr, tempFile, deviceId, workspaceId](
                                    drogon::ReqResult              result,
                                    const drogon::HttpResponsePtr &response) {
                                // Clean up temp file
                                fs::remove(tempFile);

                                if (result != drogon::ReqResult::Ok) {
                                    return sendError(*callbackPtr,
                                                     drogon::k500InternalServerError,
                                                     "Failed to upload to robot");
                                }

                                if (response->getStatusCode() != drogon::k200OK &&
                                    response->getStatusCode() != drogon::k201Created) {
                                    return sendError(*callbackPtr,
                                                     drogon::k500InternalServerError,
                                                     "Robot import failed",
                                                     std::string(response->getBody()));
                                }

                                Json::Value responseJson;
                                responseJson["success"]        = true;
                                responseJson["message"]        = "Apply completed successfully";
                                responseJson["data"]["target"] = deviceId;
                                responseJson["data"]["workspaceId"] = workspaceId;

                                auto resp = drogon::HttpResponse::newHttpJsonResponse(responseJson);
                                resp->setStatusCode(drogon::k200OK);
                                (*callbackPtr)(resp);

                                LOG_INFO << "Apply completed: " << deviceId
                                         << " (Workspace: " << workspaceId << ")";
                            });

                } catch (const std::exception &e) {
                    LOG_ERROR << "Apply exception: " << e.what();
                    sendError(*callbackPtr,
                              drogon::k500InternalServerError,
                              "Internal server error",
                              e.what());
                }
            });
}

void api::v1::Device::backup(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                             const std::string                                     &deviceId)
{
    // Check if device exists
    auto deviceResult = services::DeviceService::readDevice(baseDir, deviceId);
    if (!deviceResult.success) {
        return sendError(callback, drogon::k404NotFound, "Device not found", deviceId);
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
            [callbackPtr, ip, deviceId, this](drogon::ReqResult              result,
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
                        [callbackPtr, deviceId, this](drogon::ReqResult              result,
                                                      const drogon::HttpResponsePtr &response) {
                            if (result != drogon::ReqResult::Ok) {
                                return sendError(*callbackPtr,
                                                 drogon::k500InternalServerError,
                                                 "Failed to download archive from robot");
                            }

                            try {
                                // Save archive to temp file
                                std::string tempDir = drogon::app().getCustomConfig()["storage"]["temp_backup_dir"].asString();
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
                                        utils::getCurrentTimestamp();
                                std::string timestampStr =
                                        timestamp.substr(0, 19);  // YYYY-MM-DDTHH:MM:SS
                                std::replace(timestampStr.begin(), timestampStr.end(), 'T', '_');
                                std::replace(timestampStr.begin(), timestampStr.end(), ':', '-');
                                std::string workspaceName = deviceId + "_" + timestampStr;

                                // Create workspace metadata
                                services::WorkspaceMetadata metadata;
                                metadata.id          = workspaceId;
                                metadata.target      = deviceId;
                                metadata.name        = workspaceName;
                                metadata.description = "backup from " + deviceId;
                                metadata.createdAt   = timestamp;
                                metadata.updatedAt   = timestamp;

                                // Import workspace into device folder
                                std::string deviceBaseDir = baseDir + deviceId + "/";
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

                                LOG_INFO << "Backup completed: " << deviceId
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
