#include "DiffController.h"

#include <filesystem>

#include "../models/FileRequest.h"
#include "../services/DiffService.h"
#include "../services/FileService.h"

using namespace api::v1;
using namespace drogon_model;
using namespace services;
namespace fs = std::filesystem;

std::string baseDir = "/tmp/drogon-app/temp/";

void Diff::diffFiles(const HttpRequestPtr                          &req,
                     std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto fileReq = FileRequest::fromJson(*json);
    if (!fileReq.has_value() || !fileReq->isValid()) {
        Json::Value error;
        error["error"] = "Missing or invalid fields: filePathA, filePathB";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string fullPathA = baseDir + fileReq->filePathA;
    std::string fullPathB = baseDir + fileReq->filePathB;

    LOG_DEBUG << "diff api called";
    LOG_DEBUG << "filePathA: " << fullPathA;
    LOG_DEBUG << "filePathB: " << fullPathB;

    auto resultA = FileService::readFile(fullPathA);
    if (!resultA.success) {
        Json::Value error;
        error["error"]   = "Failed to read file A";
        error["message"] = resultA.errorMessage;
        error["path"]    = fileReq->filePathA;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    auto resultB = FileService::readFile(fullPathB);
    if (!resultB.success) {
        Json::Value error;
        error["error"]   = "Failed to read file B";
        error["message"] = resultB.errorMessage;
        error["path"]    = fileReq->filePathB;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    // Extract content from results
    std::string contentA = resultA.data["content"].asString();
    std::string contentB = resultB.data["content"].asString();

    // Get file names
    std::string nameA = fs::path(fullPathA).filename().string();
    std::string nameB = fs::path(fullPathB).filename().string();

    // Perform diff using DiffService
    auto diffResult = DiffService::diff(contentA, contentB, nameA, nameB);

    if (!diffResult.success) {
        Json::Value error;
        error["error"]   = "Failed to perform diff";
        error["message"] = diffResult.errorMessage;
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["data"]    = diffResult.data;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k200OK);
    callback(resp);

    LOG_INFO << "Successfully performed diff between " << nameA << " and " << nameB;
}

void Diff::diffWorkspaces(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback)
{
}