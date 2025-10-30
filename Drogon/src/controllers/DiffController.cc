#include "DiffController.h"

#include <filesystem>

#include "../services/DiffService.h"
#include "../services/FileService.h"
#include "ControllerHelper.h"

namespace fs = std::filesystem;

static std::string baseDir = drogon::app().getCustomConfig()["storage"]["base_dir"].asString();

using helpers::sendError;
using helpers::sendSuccess;

void api::v1::Diff::diffFiles(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON");
    }

    // Parse and validate JSON directly in controller
    if (!json->isMember("filePathA") || !json->isMember("filePathB")) {
        return sendError(callback, drogon::k400BadRequest, "Missing required fields: filePathA, filePathB");
    }

    std::string filePathA = (*json)["filePathA"].asString();
    std::string filePathB = (*json)["filePathB"].asString();

    if (filePathA.empty() || filePathB.empty()) {
        return sendError(callback, drogon::k400BadRequest, "filePathA and filePathB cannot be empty");
    }

    std::string fullPathA = baseDir + filePathA;
    std::string fullPathB = baseDir + filePathB;

    LOG_DEBUG << "diff api called";
    LOG_DEBUG << "filePathA: " << fullPathA;
    LOG_DEBUG << "filePathB: " << fullPathB;

    auto resultA = services::FileService::readFile(fullPathA);
    if (!resultA.success) {
        return sendError(callback, drogon::k404NotFound, "Failed to read file A", resultA.errorMessage + " (path: " + filePathA + ")");
    }

    auto resultB = services::FileService::readFile(fullPathB);
    if (!resultB.success) {
        return sendError(callback, drogon::k404NotFound, "Failed to read file B", resultB.errorMessage + " (path: " + filePathB + ")");
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