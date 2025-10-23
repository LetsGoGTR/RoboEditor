#include "WorkspaceController.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "../services/WorkspaceService.h"

using namespace api::v1;
using namespace services;
namespace fs = std::filesystem;

static std::string baseDir = "/tmp/drogon-app/temp/";

void Workspace::workspaceImport(const HttpRequestPtr                          &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0) {
        Json::Value error;
        error["error"] = "Failed to parse multipart data";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto files = fileUpload.getFiles();
    if (files.empty()) {
        Json::Value error;
        error["error"] = "No file uploaded";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto       &file             = files[0];
    std::string originalFilename = file.getFileName();

    // Check if archive format is supported
    if (!WorkspaceService::isSupportedArchive(originalFilename)) {
        Json::Value error;
        error["error"] = "Unsupported file format. Supported: .zip, .tar, .tar.gz, .tgz, etc.";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // Get workspace name from query parameter or use original filename
    std::string workspaceName = req->getParameter("name");
    if (workspaceName.empty()) {
        // Use original filename without extension as workspace name
        workspaceName = fs::path(originalFilename).stem().string();
    }

    std::string workspacePath = baseDir + workspaceName;

    // Check if workspace already exists
    if (fs::exists(workspacePath)) {
        Json::Value error;
        error["error"]   = "Workspace already exists";
        error["message"] = "Workspace with name '" + workspaceName + "' already exists";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k409Conflict);
        callback(resp);
        return;
    }

    try {
        // Create temporary directory for uploaded file
        std::string tempDir = "/tmp/drogon-upload/";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string tempFilePath = tempDir + drogon::utils::getUuid() + "_" + originalFilename;

        // Save uploaded file
        file.saveAs(tempFilePath);
        LOG_INFO << "File saved to: " << tempFilePath;

        // Extract archive using WorkspaceService
        auto result = WorkspaceService::importWorkspace(tempFilePath, workspacePath);

        // Remove temporary file
        fs::remove(tempFilePath);

        if (!result.success) {
            // Clean up workspace directory if extraction failed
            if (fs::exists(workspacePath)) {
                fs::remove_all(workspacePath);
            }

            Json::Value error;
            error["error"]   = "Failed to import workspace";
            error["message"] = result.errorMessage;
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        // Success response
        Json::Value response;
        response["success"] = true;
        response["message"] = "Workspace imported successfully";
        response["data"]    = result.data;

        auto resp = HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(k201Created);
        callback(resp);

        LOG_INFO << "Workspace imported: " << workspaceName;

    } catch (const std::exception &e) {
        LOG_ERROR << "Exception during import: " << e.what();

        // Clean up
        if (fs::exists(workspacePath)) {
            fs::remove_all(workspacePath);
        }

        Json::Value error;
        error["error"]   = "Internal server error";
        error["message"] = e.what();
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void Workspace::workspaceExport(const HttpRequestPtr                          &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        Json::Value error;
        error["error"] = "Missing 'name' field in request body";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string workspaceName = (*json)["name"].asString();
    std::string workspacePath = baseDir + workspaceName;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        Json::Value error;
        error["error"]   = "Workspace not found";
        error["message"] = "Workspace '" + workspaceName + "' does not exist";
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    try {
        // Create temporary directory for export
        std::string tempDir = "/tmp/drogon-export/";
        if (!fs::exists(tempDir)) {
            fs::create_directories(tempDir);
        }

        std::string outputFilename = workspaceName + ".tar.gz";
        std::string outputPath     = tempDir + outputFilename;

        // Export workspace using WorkspaceService
        auto result = WorkspaceService::exportWorkspace(workspacePath, outputPath);

        if (!result.success) {
            Json::Value error;
            error["error"]   = "Failed to export workspace";
            error["message"] = result.errorMessage;
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        // Read the compressed file
        std::ifstream file(outputPath, std::ios::binary);
        if (!file) {
            Json::Value error;
            error["error"] = "Failed to read exported file";
            auto resp      = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        std::string fileContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();

        // Remove temporary file
        fs::remove(outputPath);

        // Return file as download
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody(fileContent);
        resp->setContentTypeCode(CT_APPLICATION_OCTET_STREAM);
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + outputFilename + "\"");
        resp->setStatusCode(k200OK);
        callback(resp);

        LOG_INFO << "Workspace exported: " << workspaceName;

    } catch (const std::exception &e) {
        LOG_ERROR << "Exception during export: " << e.what();

        Json::Value error;
        error["error"]   = "Internal server error";
        error["message"] = e.what();
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void Workspace::workspaceList(const HttpRequestPtr                          &req,
                              std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto result = WorkspaceService::listWorkspaces(baseDir);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to list workspaces";
        error["message"] = result.errorMessage;
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

    LOG_INFO << "Listed workspaces: " << result.data["count"].asInt() << " workspaces found";
}
