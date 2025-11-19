#include "RepoController.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/ConfigUtils.h"
#include "../utils/TimeUtils.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

using helpers::sendError;
using helpers::sendSuccess;
using utils::config::getBaseDir;

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
        metadata.ip       = json->isMember("ip") ? (*json)["ip"].asString() : "";
        metadata.apiPort  = json->isMember("apiPort") ? (*json)["apiPort"].asInt() : 80;
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
        metadata.ip           = json->isMember("ip") ? (*json)["ip"].asString()
                                                     : existingResult.data["ip"].asString();
        metadata.apiPort      = json->isMember("apiPort") ? (*json)["apiPort"].asInt()
                                                          : existingResult.data["apiPort"].asInt();
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
