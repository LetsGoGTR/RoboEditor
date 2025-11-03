#include "DiffController.h"

#include <filesystem>

#include "../models/Request.h"
#include "../services/DiffService.h"
#include "../services/FileService.h"
#include "../utils/ConfigUtils.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

using helpers::sendError;
using helpers::sendSuccess;
using utils::config::getBaseDir;

void api::v1::Diff::diffFiles(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON");
    }

    // Parse and validate JSON directly in controller
    if (!json->isMember("filePathA") || !json->isMember("filePathB")) {
        return sendError(
                callback, drogon::k400BadRequest, "Missing required fields: filePathA, filePathB");
    }

    std::string filePathA = (*json)["filePathA"].asString();
    std::string filePathB = (*json)["filePathB"].asString();

    if (filePathA.empty() || filePathB.empty()) {
        return sendError(
                callback, drogon::k400BadRequest, "filePathA and filePathB cannot be empty");
    }

    std::string fullPathA = getBaseDir() + filePathA;
    std::string fullPathB = getBaseDir() + filePathB;

    LOG_DEBUG << "diff api called";
    LOG_DEBUG << "filePathA: " << fullPathA;
    LOG_DEBUG << "filePathB: " << fullPathB;

    auto resultA = services::FileService::readFile(fullPathA);
    if (!resultA.success) {
        return sendError(callback,
                         drogon::k404NotFound,
                         "Failed to read file A",
                         resultA.errorMessage + " (path: " + filePathA + ")");
    }

    auto resultB = services::FileService::readFile(fullPathB);
    if (!resultB.success) {
        return sendError(callback,
                         drogon::k404NotFound,
                         "Failed to read file B",
                         resultB.errorMessage + " (path: " + filePathB + ")");
    }

    // Extract content from results
    std::string contentA = resultA.data["content"].asString();
    std::string contentB = resultB.data["content"].asString();

    // Get file names
    std::string nameA = fs::path(fullPathA).filename().string();
    std::string nameB = fs::path(fullPathB).filename().string();

    // Perform diff using DiffService
    auto diffResult = services::DiffService::diff(contentA, contentB, nameA, nameB);

    if (!diffResult.success) {
        return sendError(callback,
                         drogon::k400BadRequest,
                         "Failed to perform diff",
                         diffResult.errorMessage);
    }

    sendSuccess(callback, drogon::k200OK, diffResult.data);
    LOG_INFO << "Successfully performed diff between " << nameA << " and " << nameB;
}

void api::v1::Diff::diffWorkspaces(const drogon::HttpRequestPtr                          &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value error;
        error["error"] = "Invalid JSON";
        auto resp      = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto wsReq = drogon_model::WorkspaceRequest::fromJson(*json);
    if (!wsReq.has_value() || !wsReq->isValid()) {
        Json::Value error;
        error["error"] = "Missing or invalid fields: dirPathA, dirPathB";
        auto resp      = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // ---- 경로 조립 & canonical ----
    fs::path joinedA = fs::path(getBaseDir()) / wsReq->dirPathA;
    fs::path joinedB = fs::path(getBaseDir()) / wsReq->dirPathB;

    std::error_code ec;
    auto            normalize = [&](const fs::path &p) {
        fs::path w = fs::weakly_canonical(p, ec);
        if (ec) {
            ec.clear();  // 다음 호출에 영향을 주지 않게
            w = fs::absolute(p).lexically_normal();
        }
        return w;
    };

    fs::path fullA = normalize(joinedA);
    fs::path fullB = normalize(joinedB);

    // 디렉터리 존재/타입 검증
    auto ensureDir = [](const fs::path &p, const char *which, Json::Value &out) -> bool {
        std::error_code e;
        if (!fs::exists(p, e)) {
            out["error"] = std::string(which) + " not found";
            out["path"]  = p.generic_string();
            return false;
        }
        if (!fs::is_directory(p, e)) {
            out["error"] = std::string(which) + " is not a directory";
            out["path"]  = p.generic_string();
            return false;
        }
        return true;
    };

    {
        Json::Value e1;
        if (!ensureDir(fullA, "dirA", e1)) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(e1);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }
    }
    {
        Json::Value e2;
        if (!ensureDir(fullB, "dirB", e2)) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(e2);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }
    }

    auto diffResult =
            services::DiffService::diffDirectories(fullA.generic_string(), fullB.generic_string());
    if (!diffResult.success) {
        Json::Value error;
        error["error"]   = "Failed to perform workspace diff";
        error["message"] = diffResult.errorMessage;
        auto resp        = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    Json::Value response;
    response["success"] = true;
    response["data"]    = diffResult.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Successfully performed workspace diff between " << fullA.generic_string()
             << " and " << fullB.generic_string();
}