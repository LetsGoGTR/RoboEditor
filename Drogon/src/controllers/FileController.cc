#include "FileController.h"

#include "../services/FileService.h"

using namespace api::v1;
using namespace services;

static std::string baseDir = "/tmp/drogon-app/temp/";

void File::fileRead(const HttpRequestPtr                          &req,
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

    auto result = FileService::readFile(path);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to read file";
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

    LOG_INFO << "File read: " << path;
}

void File::fileCreate(const HttpRequestPtr                          &req,
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

    auto json = req->getJsonObject();
    if (!json || !json->isMember("content")) {
        Json::Value error;
        error["error"] = "Missing 'content' field in request body";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string content = (*json)["content"].asString();

    auto result = FileService::createFile(path, content);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to create file";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k409Conflict);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "File created successfully";
    response["data"]    = result.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

    LOG_INFO << "File created: " << path;
}

void File::fileUpdate(const HttpRequestPtr                          &req,
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

    // Get content from request body
    auto json = req->getJsonObject();
    if (!json || !json->isMember("content")) {
        Json::Value error;
        error["error"] = "Missing 'content' field in request body";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string content = (*json)["content"].asString();

    auto result = FileService::updateFile(path, content);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to update file";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "File updated successfully";
    response["data"]    = result.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "File updated: " << path;
}

void File::fileDelete(const HttpRequestPtr                          &req,
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

    auto result = FileService::deleteFile(path);

    if (!result.success) {
        Json::Value error;
        error["error"]   = "Failed to delete file";
        error["message"] = result.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["message"] = "File deleted successfully";

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "File deleted: " << path;
}