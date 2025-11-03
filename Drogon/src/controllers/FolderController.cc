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
    auto path = getBaseDir() + req->getParameter("path");

    if (path.empty()) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' query parameter");
    }

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
    auto path = getBaseDir() + req->getParameter("path");

    if (path.empty()) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' query parameter");
    }

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
    auto oldPath = getBaseDir() + req->getParameter("path");

    if (oldPath.empty()) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' query parameter");
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("newPath")) {
        return sendError(
                callback, drogon::k400BadRequest, "Missing 'newPath' field in request body");
    }

    std::string newPath = getBaseDir() + (*json)["newPath"].asString();

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
    auto path = getBaseDir() + req->getParameter("path");

    if (path.empty()) {
        return sendError(callback, drogon::k400BadRequest, "Missing 'path' query parameter");
    }

    auto result = services::FolderService::deleteFolder(path);

    if (!result.success) {
        return sendError(
                callback, drogon::k404NotFound, "Failed to delete folder", result.errorMessage);
    }

    Json::Value data;
    sendSuccess(callback, drogon::k200OK, data, "Folder deleted successfully");
    LOG_INFO << "Folder deleted: " << path;
}
