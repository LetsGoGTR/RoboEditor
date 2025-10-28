#include "FileController.h"

#include "../services/FileService.h"

static std::string baseDir = "/tmp/drogon-app/storage/";

void api::v1::File::fileRead(
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

  auto result = services::FileService::readFile(path);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to read file";
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

  LOG_INFO << "File read: " << path;
}

void api::v1::File::fileCreate(
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

  auto json = req->getJsonObject();
  if (!json || !json->isMember("content")) {
    Json::Value error;
    error["error"] = "Missing 'content' field in request body";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  std::string content = (*json)["content"].asString();

  auto result = services::FileService::createFile(path, content);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to create file";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k409Conflict);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "File created successfully";
  response["data"] = result.data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k201Created);
  callback(resp);

  LOG_INFO << "File created: " << path;
}

void api::v1::File::fileUpdate(
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

  // Get content from request body
  auto json = req->getJsonObject();
  if (!json || !json->isMember("content")) {
    Json::Value error;
    error["error"] = "Missing 'content' field in request body";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  std::string content = (*json)["content"].asString();

  auto result = services::FileService::updateFile(path, content);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to update file";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "File updated successfully";
  response["data"] = result.data;

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);

  LOG_INFO << "File updated: " << path;
}

void api::v1::File::fileDelete(
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

  auto result = services::FileService::deleteFile(path);

  if (!result.success) {
    Json::Value error;
    error["error"] = "Failed to delete file";
    error["message"] = result.errorMessage;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  Json::Value response;
  response["success"] = true;
  response["message"] = "File deleted successfully";

  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  resp->setStatusCode(drogon::k200OK);
  callback(resp);

  LOG_INFO << "File deleted: " << path;
}