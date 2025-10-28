#include "WorkspaceController.h"

#include <filesystem>
#include <fstream>
#include <sstream>

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

void api::v1::Workspace::workspaceImport(
        const drogon::HttpRequestPtr                          &req,
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
    metadata.id          = drogon::utils::getUuid();
    metadata.target      = req->getParameter("target");
    metadata.name        = workspaceName;
    metadata.description = req->getParameter("description");
    metadata.createdAt   = services::WorkspaceService::getCurrentTimestamp();
    metadata.updatedAt   = metadata.createdAt;

    try {
        std::string tempDir = "/tmp/drogon-upload/";
        if (!fs::exists(tempDir))
            fs::create_directories(tempDir);

        std::string tempFile = tempDir + drogon::utils::getUuid() + "_" + filename;
        file.saveAs(tempFile);

        auto result = services::WorkspaceService::importWorkspace(tempFile, baseDir, metadata);
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

        LOG_INFO << "Imported: " << workspaceName << " (ID: " << metadata.id << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Import exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Workspace::workspaceExport(
        const drogon::HttpRequestPtr                          &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("id"))
        return sendError(callback, drogon::k400BadRequest, "Missing 'id' field");

    std::string workspaceId   = (*json)["id"].asString();
    std::string workspacePath = baseDir + workspaceId;

    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath))
        return sendError(callback, drogon::k404NotFound, "Workspace not found", workspaceId);

    try {
        auto        metadata      = services::WorkspaceService::loadMetadata(workspacePath);
        std::string workspaceName = metadata.name.empty() ? workspaceId : metadata.name;

        std::string tempDir = "/tmp/drogon-export/";
        if (!fs::exists(tempDir))
            fs::create_directories(tempDir);

        std::string outputFilename = workspaceName + ".tar.gz";
        std::string outputPath     = tempDir + outputFilename;

        auto result = services::WorkspaceService::exportWorkspace(workspaceId, baseDir, outputPath);

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

void api::v1::Workspace::workspaceList(
        const drogon::HttpRequestPtr                          &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto result = services::WorkspaceService::listWorkspaces(baseDir);

    if (!result.success)
        return sendError(callback,
                         drogon::k500InternalServerError,
                         "Failed to list workspaces",
                         result.errorMessage);

    Json::Value response;
    response["success"] = true;
    response["data"]    = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Listed " << result.data["count"].asInt() << " workspaces";
}

void api::v1::Workspace::create(const drogon::HttpRequestPtr                          &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("name"))
        return sendError(callback, drogon::k400BadRequest, "Missing required field: name");

    services::WorkspaceMetadata metadata;
    metadata.id          = drogon::utils::getUuid();
    metadata.name        = (*json)["name"].asString();
    metadata.target      = json->isMember("target") ? (*json)["target"].asString() : "";
    metadata.description = json->isMember("description") ? (*json)["description"].asString() : "";

    try {
        auto result = services::WorkspaceService::createWorkspace(baseDir, metadata);

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

        LOG_INFO << "Created workspace: " << metadata.name << " (ID: " << metadata.id << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "Create exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}

void api::v1::Workspace::info(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                              const std::string                                     &workspaceId)
{
    try {
        auto result = services::WorkspaceService::readWorkspace(baseDir, workspaceId);

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

void api::v1::Workspace::update(const drogon::HttpRequestPtr                          &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                const std::string                                     &workspaceId)
{
    auto json = req->getJsonObject();
    if (!json)
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

    try {
        // Load existing metadata first
        auto existingResult = services::WorkspaceService::readWorkspace(baseDir, workspaceId);
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

        auto result =
                services::WorkspaceService::updateWorkspaceMetadata(baseDir, workspaceId, metadata);

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

void api::v1::Workspace::remove(const drogon::HttpRequestPtr                          &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                const std::string                                     &workspaceId)
{
    try {
        auto result = services::WorkspaceService::deleteWorkspace(baseDir, workspaceId);

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
