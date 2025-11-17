#include "FolderController.h"

#include "../services/FolderService.h"
#include "../utils/ConfigUtils.h"
#include "ControllerHelper.h"

using helpers::sendError;
using helpers::sendSuccess;
using utils::config::getBaseDir;

void api::v1::Folder::folderRead(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    }

    std::string path = (*json)["path"].asString();

    auto result = services::FolderService::readFolder(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to list folder", result.errorMessage);
    }

    sendSuccess(callback, drogon::k200OK, result.data);
    LOG_INFO << "Folder listed: " << path;
}

void api::v1::Folder::folderCreate(const drogon::HttpRequestPtr                          &req,
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

    auto result = services::FolderService::createFolder(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k409Conflict, "Failed to create folder", result.errorMessage);
    }

    sendSuccess(callback, drogon::k201Created, result.data, "Folder created successfully");
    LOG_INFO << "Folder created: " << path;
}

void api::v1::Folder::folderUpdate(const drogon::HttpRequestPtr                          &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path") || !json->isMember("newPath")) {
        return sendError(
                callback, drogon::k400BadRequest, "Missing 'path' or 'newPath' field in request body");
    }

    std::string oldPath = (*json)["path"].asString();
    std::string newPath = (*json)["newPath"].asString();

    auto result = services::FolderService::updateFolder(oldPath, newPath);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to rename folder", result.errorMessage);
    }

    sendSuccess(callback, drogon::k200OK, result.data, "Folder renamed successfully");
    LOG_INFO << "Folder renamed from: " << oldPath << " to: " << newPath;
}

void api::v1::Folder::folderDelete(const drogon::HttpRequestPtr                          &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !json->isMember("path")) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' field in request body");
    }

    std::string path = (*json)["path"].asString();

    auto result = services::FolderService::deleteFolder(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to delete folder", result.errorMessage);
    }

    Json::Value data;
    sendSuccess(callback, drogon::k200OK, data, "Folder deleted successfully");
    LOG_INFO << "Folder deleted: " << path;
}
