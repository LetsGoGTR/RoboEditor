#include "RepoController.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../services/AuthService.h"
#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/ConfigUtils.h"
#include "../utils/TimeUtils.h"
#include "../utils/robot/RobotHttpClient.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

using helpers::sendError;
using helpers::sendSuccess;
using utils::config::getBaseDir;
using utils::config::getTempApplyDir;
using utils::config::getTempBackupDir;
using utils::config::getTempExportDir;
using utils::config::getTempUploadDir;

// ============================================================================
// Device CRUD Methods
// ============================================================================

void api::v1::Repo::list(const drogon::HttpRequestPtr                          &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try {
        auto result = services::DeviceService::listDevices();

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

void api::v1::Repo::create(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    if (!json->isMember("serialNumber") || !json->isMember("name"))
        return sendError(
                callback, drogon::k400BadRequest, "Missing required fields: serialNumber, name");

    std::string serialNumber = (*json)["serialNumber"].asString();
    if (serialNumber.empty())
        return sendError(callback, drogon::k400BadRequest, "Device serialNumber cannot be empty");

    try {
        services::DeviceMetadata metadata;
        metadata.serialNumber = serialNumber;
        metadata.name         = (*json)["name"].asString();
        metadata.description =
                json->isMember("description") ? (*json)["description"].asString() : "";
        metadata.api      = json->isMember("api") ? (*json)["api"].asString() : "";
        metadata.sftpHost = json->isMember("sftpHost") ? (*json)["sftpHost"].asString() : "";
        metadata.sftpPort = json->isMember("sftpPort") ? (*json)["sftpPort"].asInt() : 22;
        metadata.sftpPassword =
                json->isMember("sftpPassword") ? (*json)["sftpPassword"].asString() : "";
        metadata.sftpUser = json->isMember("sftpUser") ? (*json)["sftpUser"].asString() : "";

        auto result = services::DeviceService::createDevice(metadata);

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

        LOG_INFO << "Created device: " << metadata.name
                 << " (serialNumber: " << metadata.serialNumber << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Create exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::info(const drogon::HttpRequestPtr                          &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                         const std::string                                     &deviceId)
{
    try {
        // Get device info
        auto deviceResult = services::DeviceService::readDevice(deviceId);

        if (!deviceResult.success)
            return sendError(
                    callback, drogon::k404NotFound, "Device not found", deviceResult.errorMessage);

        // Get workspace list for this device
        auto workspaceResult = services::WorkspaceService::listWorkspaces(deviceId);

        if (!workspaceResult.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to list workspaces",
                             workspaceResult.errorMessage);

        Json::Value response;
        response["success"]    = true;
        response["device"]     = deviceResult.data;
        response["workspaces"] = workspaceResult.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Retrieved device info: " << deviceId << " with "
                 << workspaceResult.data["count"].asInt() << " workspaces";

    } catch (const std::exception &e) {
        LOG_ERROR << "Info exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::update(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                           const std::string                                     &deviceId)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    try {
        // Load existing metadata first
        auto existingResult = services::DeviceService::readDevice(deviceId);
        if (!existingResult.success) {
            return sendError(callback, drogon::k404NotFound, "Device not found", deviceId);
        }

        // Prepare updated metadata, keeping existing values if not provided
        services::DeviceMetadata metadata;
        metadata.name         = json->isMember("name") ? (*json)["name"].asString()
                                                       : existingResult.data["name"].asString();
        metadata.description  = json->isMember("description")
                                        ? (*json)["description"].asString()
                                        : existingResult.data["description"].asString();
        metadata.api          = json->isMember("api") ? (*json)["api"].asString()
                                                      : existingResult.data["api"].asString();
        metadata.sftpHost     = json->isMember("sftpHost") ? (*json)["sftpHost"].asString()
                                                           : existingResult.data["sftpHost"].asString();
        metadata.sftpPort     = json->isMember("sftpPort") ? (*json)["sftpPort"].asInt()
                                                           : existingResult.data["sftpPort"].asInt();
        metadata.sftpPassword = json->isMember("sftpPassword")
                                        ? (*json)["sftpPassword"].asString()
                                        : existingResult.data["sftpPassword"].asString();
        metadata.sftpUser     = json->isMember("sftpUser") ? (*json)["sftpUser"].asString()
                                                           : existingResult.data["sftpUser"].asString();

        auto result = services::DeviceService::updateDevice(deviceId, metadata);

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

void api::v1::Repo::remove(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                           const std::string                                     &deviceId)
{
    try {
        auto result = services::DeviceService::deleteDevice(deviceId);

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

void api::v1::Repo::apply(const drogon::HttpRequestPtr                          &req,
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
    auto deviceResult = services::DeviceService::readDevice(deviceId);
    if (!deviceResult.success) {
        return sendError(callback, drogon::k404NotFound, "Device not found", deviceId);
    }

    // Get API URL from device metadata
    std::string ip = deviceResult.data["api"].asString();
    if (ip.empty()) {
        ip = "localhost:80";
    }

    // Wrap callback in shared_ptr for lambda capture
    auto callbackPtr = std::make_shared<std::function<void(const drogon::HttpResponsePtr &)>>(
            std::move(callback));

    // Check if robot is running before proceeding
    checkRobotStatus(
            ip,
            [callbackPtr, ip, deviceId, workspaceId, this]() {
                try {
                    // Export workspace to temp file
                    std::string tempDir = utils::config::getTempApplyDir();
                    if (!fs::exists(tempDir))
                        fs::create_directories(tempDir);

                    std::string tempFile = tempDir + drogon::utils::getUuid() + ".tar.gz";

                    // Export workspace
                    auto exportResult = services::WorkspaceService::exportWorkspace(
                            workspaceId, tempFile, deviceId);

                    if (!exportResult.success) {
                        fs::remove(tempFile);
                        return sendError(*callbackPtr,
                                         drogon::k404NotFound,
                                         "Failed to export workspace",
                                         exportResult.errorMessage);
                    }

                    // Upload workspace to robot
                    utils::RobotHttpClient::uploadWorkspace(
                            ip,
                            tempFile,
                            [callbackPtr, tempFile, deviceId, workspaceId](
                                    bool success, const std::string &error) {
                                // Clean up temp file
                                fs::remove(tempFile);

                                if (!success) {
                                    return sendError(*callbackPtr,
                                                     drogon::k500InternalServerError,
                                                     "Failed to upload workspace to robot",
                                                     error);
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
            },
            callbackPtr);
}

void api::v1::Repo::backup(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                           const std::string                                     &deviceId)
{
    // Check if device exists
    auto deviceResult = services::DeviceService::readDevice(deviceId);
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

    // Check if robot is running before proceeding
    checkRobotStatus(
            ip,
            [callbackPtr, ip, deviceId, this]() {
                // Download workspace from robot
                utils::RobotHttpClient::downloadWorkspace(
                        ip,
                        [callbackPtr, deviceId, this](const std::string &content,
                                                      const std::string &error) {
                            if (!error.empty()) {
                                return sendError(*callbackPtr,
                                                 drogon::k500InternalServerError,
                                                 "Failed to download workspace from robot",
                                                 error);
                            }

                            try {
                                // Save archive to temp file
                                std::string tempDir = utils::config::getTempBackupDir();
                                if (!fs::exists(tempDir))
                                    fs::create_directories(tempDir);

                                std::string tempFile =
                                        tempDir + drogon::utils::getUuid() + ".tar.gz";
                                std::ofstream file(tempFile, std::ios::binary);
                                file << content;
                                file.close();

                                // Generate workspace ID and name
                                std::string workspaceId = drogon::utils::getUuid();
                                std::string timestamp   = utils::getCurrentTimestamp();
                                std::string timestampStr =
                                        timestamp.substr(0, 19);  // YYYY-MM-DDTHH:MM:SS
                                std::replace(timestampStr.begin(), timestampStr.end(), 'T', '_');
                                std::replace(timestampStr.begin(), timestampStr.end(), ':', '-');
                                std::string workspaceName = deviceId + "_" + timestampStr;

                                // Create workspace metadata
                                services::WorkspaceMetadata metadata;
                                metadata.uuid        = workspaceId;
                                metadata.target      = deviceId;
                                metadata.name        = workspaceName;
                                metadata.description = "backup from " + deviceId;
                                metadata.createdAt   = timestamp;
                                metadata.updatedAt   = timestamp;

                                // Import workspace into device folder
                                auto importResult = services::WorkspaceService::importWorkspace(
                                        tempFile, metadata, deviceId);

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
            },
            callbackPtr);
}

void api::v1::Repo::checkRobotStatus(
        const std::string                                                    &ip,
        std::function<void()>                                                 onNotRunning,
        std::shared_ptr<std::function<void(const drogon::HttpResponsePtr &)>> callbackPtr)
{
    utils::RobotHttpClient::checkRunning(
            ip, [callbackPtr, onNotRunning](bool isRunning, const std::string &error) {
                if (!error.empty()) {
                    return sendError(*callbackPtr,
                                     drogon::k500InternalServerError,
                                     "Failed to check robot status",
                                     error);
                }

                if (isRunning) {
                    return sendError(*callbackPtr,
                                     drogon::k409Conflict,
                                     "Robot is running",
                                     "Cannot perform operation while robot is running");
                }

                // Robot is not running, proceed with operation
                onNotRunning();
            });
}

// ============================================================================
// Workspace CRUD Methods
// ============================================================================

void api::v1::Repo::createWorkspace(const drogon::HttpRequestPtr                          &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    const std::string                                     &deviceId)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("name"))
        return sendError(callback, drogon::k400BadRequest, "Missing required field: name");

    services::WorkspaceMetadata metadata;
    metadata.uuid        = drogon::utils::getUuid();
    metadata.name        = (*json)["name"].asString();
    metadata.target      = deviceId;
    metadata.description = json->isMember("description") ? (*json)["description"].asString() : "";

    try {
        auto result = services::WorkspaceService::createWorkspace(metadata, deviceId);

        if (!result.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to create workspace",
                             result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Workspace created successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);

        LOG_INFO << "Created workspace: " << metadata.name << " (ID: " << metadata.uuid << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Create exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::workspaceInfo(const drogon::HttpRequestPtr                          &req,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                  const std::string                                     &deviceId,
                                  const std::string &workspaceId)
{
    try {
        auto result = services::WorkspaceService::readWorkspace(workspaceId, deviceId);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Workspace not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Retrieved workspace: " << workspaceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Get exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::workspaceUpdate(const drogon::HttpRequestPtr                          &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    const std::string                                     &deviceId,
                                    const std::string &workspaceId)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    try {
        // Load existing metadata first
        auto existingResult = services::WorkspaceService::readWorkspace(workspaceId, deviceId);
        if (!existingResult.success) {
            return sendError(callback, drogon::k404NotFound, "Workspace not found", workspaceId);
        }

        // Prepare updated metadata, keeping existing values if not provided
        services::WorkspaceMetadata metadata;
        metadata.name        = json->isMember("name") ? (*json)["name"].asString()
                                                      : existingResult.data["metadata"]["name"].asString();
        metadata.target      = json->isMember("target")
                                       ? (*json)["target"].asString()
                                       : existingResult.data["metadata"]["target"].asString();
        metadata.description = json->isMember("description")
                                       ? (*json)["description"].asString()
                                       : existingResult.data["metadata"]["description"].asString();

        auto result = services::WorkspaceService::updateWorkspace(workspaceId, metadata, deviceId);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Workspace not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Workspace updated successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Updated workspace: " << workspaceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Update exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::workspaceRemove(const drogon::HttpRequestPtr                          &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                    const std::string                                     &deviceId,
                                    const std::string &workspaceId)
{
    try {
        auto result = services::WorkspaceService::deleteWorkspace(workspaceId, deviceId);

        if (!result.success)
            return sendError(
                    callback, drogon::k404NotFound, "Workspace not found", result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Workspace deleted successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Deleted workspace: " << workspaceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Delete exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::workspaceImport(const drogon::HttpRequestPtr                          &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    drogon::MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0)
        return sendError(callback, drogon::k400BadRequest, "Failed to parse multipart data");

    auto files = fileUpload.getFiles();
    if (files.empty())
        return sendError(callback, drogon::k400BadRequest, "No file uploaded");

    auto       &file     = files[0];
    std::string filename = file.getFileName();

    if (!services::WorkspaceService::isSupportedArchive(filename))
        return sendError(callback, drogon::k400BadRequest, "Unsupported file format");

    // Build metadata
    std::string workspaceName = req->getParameter("name");
    if (workspaceName.empty())
        workspaceName = fs::path(filename).stem().string();

    services::WorkspaceMetadata metadata;
    metadata.uuid        = drogon::utils::getUuid();
    metadata.target      = req->getParameter("target");
    metadata.name        = workspaceName;
    metadata.description = req->getParameter("description");
    metadata.createdAt   = utils::getCurrentTimestamp();
    metadata.updatedAt   = metadata.createdAt;

    try {
        std::string tempDir = getTempUploadDir();
        if (!fs::exists(tempDir))
            fs::create_directories(tempDir);

        std::string tempFile = tempDir + drogon::utils::getUuid() + "_" + filename;
        file.saveAs(tempFile);

        auto result =
                services::WorkspaceService::importWorkspace(tempFile, metadata, metadata.target);
        fs::remove(tempFile);

        if (!result.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to import workspace",
                             result.errorMessage);

        Json::Value response;
        response["success"] = true;
        response["message"] = "Workspace imported successfully";
        response["data"]    = result.data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);

        LOG_INFO << "Imported: " << workspaceName << " (ID: " << metadata.uuid << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Import exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Repo::workspaceExport(const drogon::HttpRequestPtr                          &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("id"))
        return sendError(callback, drogon::k400BadRequest, "Missing 'id' field");

    std::string workspaceId = (*json)["id"].asString();

    try {
        // Read workspace to get metadata
        auto readResult = services::WorkspaceService::readWorkspace(workspaceId);
        if (!readResult.success)
            return sendError(callback, drogon::k404NotFound, "Workspace not found", workspaceId);

        auto        metadata = services::WorkspaceMetadata::fromJson(readResult.data["metadata"]);
        std::string workspaceName = metadata.name.empty() ? workspaceId : metadata.name;

        std::string tempDir = getTempExportDir();
        if (!fs::exists(tempDir))
            fs::create_directories(tempDir);

        std::string outputFilename = workspaceName + ".tar.gz";
        std::string outputPath     = tempDir + outputFilename;

        auto result = services::WorkspaceService::exportWorkspace(workspaceId, outputPath);

        if (!result.success)
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to export workspace",
                             result.errorMessage);

        std::ifstream file(outputPath, std::ios::binary);
        if (!file)
            return sendError(
                    callback, drogon::k500InternalServerError, "Failed to read exported file");

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        fs::remove(outputPath);

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(content);
        resp->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + outputFilename + "\"");
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Exported: " << workspaceName << " (ID: " << workspaceId << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Export exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}
