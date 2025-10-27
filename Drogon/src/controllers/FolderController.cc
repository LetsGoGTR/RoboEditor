#include "FolderController.h"

#include "../services/FolderService.h"

static std::string baseDir = "/tmp/drogon-app/storage/";

void api::v1::Folder::folderRead(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto path = baseDir + req->getParameter("path");

  if (path.empty()) {
    Json::Value error;
    error["error"] = "Missing 'path' query parameter";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto result = services::FolderService::readFolder(path);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to list folder";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["data"] = result.data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);

  LOG_INFO << "Folder listed: " << path;
}

void api::v1::Folder::folderCreate(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto path = baseDir + req->getParameter("path");

  if (path.empty()) {
    Json::Value error;
    error["error"] = "Missing 'path' query parameter";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto result = services::FolderService::createFolder(path);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to create folder";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k409Conflict);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "Folder created successfully";
  response["data"] = result.data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k201Created);
  callback(resp);

  LOG_INFO << "Folder created: " << path;
}

void api::v1::Folder::folderUpdate(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto oldPath = baseDir + req->getParameter("path");

  if (oldPath.empty()) {
    Json::Value error;
    error["error"] = "Missing 'path' query parameter";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto json = req->getJsonObject();
  if (!json || !json->isMember("newPath")) {
    Json::Value error;
    error["error"] = "Missing 'newPath' field in request body";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  std::string newPath = baseDir + (*json)["newPath"].asString();

  auto result = services::FolderService::updateFolder(oldPath, newPath);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to rename folder";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "Folder renamed successfully";
  response["data"] = result.data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);

  LOG_INFO << "Folder renamed from: " << oldPath << " to: " << newPath;
}

void api::v1::Folder::folderDelete(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto path = baseDir + req->getParameter("path");

  if (path.empty()) {
    Json::Value error;
    error["error"] = "Missing 'path' query parameter";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto result = services::FolderService::deleteFolder(path);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to delete folder";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "Folder deleted successfully";

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);

  LOG_INFO << "Folder deleted: " << path;
}
