#include "FolderController.h"

#include "../services/FolderService.h"

using namespace api::v1;
using namespace services;

static std::string baseDir = "/tmp/drogon-app/temp/";

void Folder::folderRead(const HttpRequestPtr                          &req,
                        std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto path = baseDir + req->getParameter("path");

    if (path.empty()) {
        Json::Value error;
        error["error"] = "Missing 'path' query parameter";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto result = FolderService::readFolder(path);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to list folder";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["data"]    = result.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "Folder listed: " << path;
}

void Folder::folderCreate(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto path = baseDir + req->getParameter("path");

    if (path.empty()) {
        Json::Value error;
        error["error"] = "Missing 'path' query parameter";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto result = FolderService::createFolder(path);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to create folder";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k409Conflict);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "Folder created successfully";
    response["data"]    = result.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

    LOG_INFO << "Folder created: " << path;
}

void Folder::folderUpdate(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto oldPath = baseDir + req->getParameter("path");

    if (oldPath.empty()) {
        Json::Value error;
        error["error"] = "Missing 'path' query parameter";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("newPath")) {
        Json::Value error;
        error["error"] = "Missing 'newPath' field in request body";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string newPath = baseDir + (*json)["newPath"].asString();

    auto result = FolderService::updateFolder(oldPath, newPath);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to rename folder";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "Folder renamed successfully";
    response["data"]    = result.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "Folder renamed from: " << oldPath << " to: " << newPath;
}

void Folder::folderDelete(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto path = baseDir + req->getParameter("path");

    if (path.empty()) {
        Json::Value error;
        error["error"] = "Missing 'path' query parameter";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto result = FolderService::deleteFolder(path);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to delete folder";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "Folder deleted successfully";

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "Folder deleted: " << path;
}
