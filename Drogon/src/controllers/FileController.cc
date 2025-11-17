#include "FileController.h"

#include "../services/FileService.h"
#include "../utils/ConfigUtils.h"
#include "ControllerHelper.h"

using helpers::sendError;
using helpers::sendSuccess;
using utils::config::getBaseDir;

void api::v1::File::fileRead(const drogon::HttpRequestPtr                          &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    }

    std::string path = (*json)["path"].asString();

    auto result = services::FileService::readFile(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to read file", result.errorMessage);
    }

    sendSuccess(callback, drogon::k200OK, result.data);
    LOG_INFO << "File read: " << path;
}

void api::v1::File::fileCreate(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path") || !json->isMember("content")) {
        return sendError(
                callback, drogon::k400BadRequest, "Missing 'path' or 'content' field in request body");
    }

    std::string path    = (*json)["path"].asString();
    std::string path    = (*json)["path"].asString();
    std::string content = (*json)["content"].asString();

    auto result = services::FileService::createFile(path, content);

    if (!result.success) {
        return sendError(
                callback, drogon::k409Conflict, "Failed to create file", result.errorMessage);
    }

    sendSuccess(callback, drogon::k201Created, result.data, "File created successfully");
    LOG_INFO << "File created: " << path;
}

void api::v1::File::fileUpdate(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    }

    std::string path = (*json)["path"].asString();

    std::string path = (*json)["path"].asString();

    bool hasContent = json->isMember("content");
    bool hasNewPath = json->isMember("newPath");

    if (!hasContent && !hasNewPath) {
        return sendError(
                callback, drogon::k400BadRequest, "Either 'content' or 'newPath' must be provided");
    }

    std::string             currentPath = path;
    services::ServiceResult result;

    // Move file if newPath is provided
    if (hasNewPath) {
        std::string newPath = (*json)["newPath"].asString();
        result              = services::FileService::moveFile(currentPath, newPath);

        if (!result.success) {
            return sendError(
                    callback, drogon::k404NotFound, "Failed to move file", result.errorMessage);
        }

        currentPath = newPath;  // Update path for content update
    }

    // Update content if provided
    if (hasContent) {
        std::string content = (*json)["content"].asString();
        result              = services::FileService::updateFile(currentPath, content);

        if (!result.success) {
            return sendError(callback,
                             drogon::k500InternalServerError,
                             "Failed to update file content",
                             result.errorMessage);
        }
    }

    // Success response - use file info from service result
    Json::Value data;
    data["path"] = currentPath;
    data["info"] = result.data["info"];

    sendSuccess(callback, drogon::k200OK, data, "File updated successfully");
    LOG_INFO << "File updated: " << path << (hasNewPath ? " -> " + currentPath : "");
}

void api::v1::File::fileDelete(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    }

    std::string path = (*json)["path"].asString();

    std::string path = (*json)["path"].asString();

    auto result = services::FileService::deleteFile(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to delete file", result.errorMessage);
    }

    Json::Value data;
    sendSuccess(callback, drogon::k200OK, data, "File deleted successfully");
    LOG_INFO << "File deleted: " << path;
}
