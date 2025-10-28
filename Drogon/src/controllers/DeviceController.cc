#include "DeviceController.h"

#include <filesystem>

#include "../services/DeviceService.h"

namespace fs = std::filesystem;

static std::string baseDir = "/tmp/drogon-app/storage/";

// Helper functions
static void sendError(
    std::function<void(const drogon::HttpResponsePtr&)>& callback,
    drogon::HttpStatusCode status, const std::string& error,
    const std::string& message = "") {
  Json::Value json;
  json["error"] = error;
  if (!message.empty()) json["message"] = message;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(status);
  callback(resp);
}

void api::v1::Device::create(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto json = req->getJsonObject();
  if (!json)
    return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

  if (!json->isMember("id") || !json->isMember("name"))
    return sendError(callback, drogon::k400BadRequest,
                     "Missing required fields: id, name");

  std::string deviceId = (*json)["id"].asString();
  if (deviceId.empty())
    return sendError(callback, drogon::k400BadRequest, "Device ID cannot be empty");

  try {
    services::DeviceMetadata metadata;
    metadata.id = deviceId;
    metadata.name = (*json)["name"].asString();
    metadata.description =
        json->isMember("description") ? (*json)["description"].asString() : "";
    metadata.ip = json->isMember("ip") ? (*json)["ip"].asString() : "";

    auto result = services::DeviceService::createDevice(baseDir, metadata);

    if (!result.success)
      return sendError(callback, drogon::k500InternalServerError,
                       "Failed to create device", result.errorMessage);

    Json::Value response;
    response["success"] = true;
    response["message"] = "Device created successfully";
    response["data"] = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);

    LOG_INFO << "Created device: " << metadata.name
             << " (ID: " << metadata.id << ")";

  } catch (const std::exception& e) {
    LOG_ERROR << "Create exception: " << e.what();
    sendError(callback, drogon::k500InternalServerError,
              "Internal server error", e.what());
  }
}

void api::v1::Device::info(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {
  try {
    auto result = services::DeviceService::getDeviceInfo(baseDir, target);

    if (!result.success)
      return sendError(callback, drogon::k404NotFound, "Device not found",
                       result.errorMessage);

    Json::Value response;
    response["success"] = true;
    response["data"] = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Retrieved device info: " << target;

  } catch (const std::exception& e) {
    LOG_ERROR << "Info exception: " << e.what();
    sendError(callback, drogon::k500InternalServerError,
              "Internal server error", e.what());
  }
}

void api::v1::Device::update(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {
  auto json = req->getJsonObject();
  if (!json)
    return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");

  try {
    services::DeviceMetadata metadata;
    metadata.name = json->isMember("name") ? (*json)["name"].asString() : "";
    metadata.description =
        json->isMember("description") ? (*json)["description"].asString() : "";
    metadata.ip = json->isMember("ip") ? (*json)["ip"].asString() : "";

    auto result =
        services::DeviceService::updateDevice(baseDir, target, metadata);

    if (!result.success)
      return sendError(callback, drogon::k404NotFound, "Device not found",
                       result.errorMessage);

    Json::Value response;
    response["success"] = true;
    response["message"] = "Device updated successfully";
    response["data"] = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Updated device: " << target;

  } catch (const std::exception& e) {
    LOG_ERROR << "Update exception: " << e.what();
    sendError(callback, drogon::k500InternalServerError,
              "Internal server error", e.what());
  }
}

void api::v1::Device::remove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {
  try {
    auto result = services::DeviceService::deleteDevice(baseDir, target);

    if (!result.success)
      return sendError(callback, drogon::k404NotFound, "Device not found",
                       result.errorMessage);

    Json::Value response;
    response["success"] = true;
    response["message"] = "Device deleted successfully";
    response["data"] = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Deleted device: " << target;

  } catch (const std::exception& e) {
    LOG_ERROR << "Delete exception: " << e.what();
    sendError(callback, drogon::k500InternalServerError,
              "Internal server error", e.what());
  }
}

void api::v1::Device::upload(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {}

void api::v1::Device::download(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {}

void api::v1::Device::restore(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {}

void api::v1::Device::backup(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& target) {}
