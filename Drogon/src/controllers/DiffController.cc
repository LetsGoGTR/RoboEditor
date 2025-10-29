#include "DiffController.h"

#include <filesystem>

#include "../models/FileRequest.h"
#include "../services/DiffService.h"
#include "../services/FileService.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

static std::string baseDir = "/tmp/drogon-app/storage/";

using helpers::sendError;
using helpers::sendSuccess;

void api::v1::Diff::diffFiles(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON");
    }

    auto fileReq = drogon_model::FileRequest::fromJson(*json);
    if (!fileReq.has_value() || !fileReq->isValid()) {
        return sendError(callback, drogon::k400BadRequest, "Missing or invalid fields: filePathA, filePathB");
    }

    std::string fullPathA = baseDir + fileReq->filePathA;
    std::string fullPathB = baseDir + fileReq->filePathB;

    LOG_DEBUG << "diff api called";
    LOG_DEBUG << "filePathA: " << fullPathA;
    LOG_DEBUG << "filePathB: " << fullPathB;

    auto resultA = services::FileService::readFile(fullPathA);
    if (!resultA.success) {
        return sendError(callback, drogon::k404NotFound, "Failed to read file A", resultA.errorMessage + " (path: " + fileReq->filePathA + ")");
    }

    auto resultB = services::FileService::readFile(fullPathB);
    if (!resultB.success) {
        return sendError(callback, drogon::k404NotFound, "Failed to read file B", resultB.errorMessage + " (path: " + fileReq->filePathB + ")");
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
        return sendError(callback, drogon::k400BadRequest, "Failed to perform diff", diffResult.errorMessage);
    }

    sendSuccess(callback, drogon::k200OK, diffResult.data);
    LOG_INFO << "Successfully performed diff between " << nameA << " and " << nameB;
}

void api::v1::Diff::diffWorkspaces(const drogon::HttpRequestPtr                          &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
}