#include "DiffController.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>

#include "../services/DiffService.h"
#include "../services/FileService.h"
#include "../utils/ConfigUtils.h"

namespace fs = std::filesystem;

using helpers::makeError;
using helpers::makeSuccess;
using utils::config::getBaseDir;

namespace
{
    struct WorkspaceRequest
    {
        std::string dirPathA;
        std::string dirPathB;

        static std::optional<WorkspaceRequest> fromJson(const Json::Value &json)
        {
            if (!json.isMember("dirPathA") || !json.isMember("dirPathB")) {
                return std::nullopt;
            }

            WorkspaceRequest req;
            req.dirPathA = json["dirPathA"].asString();
            req.dirPathB = json["dirPathB"].asString();

            return req;
        }

        static bool isSafeRelative(const std::string &p)
        {
            if (p.empty())
                return false;
            // 절대경로 형태 차단 (리눅스/맥: '/', 윈도우: 드라이브 + ':', 혹은 '\\' 시작)
            if (p.size() >= 1 && (p[0] == '/' || p[0] == '\\'))
                return false;
            if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':')
                return false;
            // 상위 폴더 탈출 방지
            if (p.find("..") != std::string::npos)
                return false;
            return true;
        }

        bool isValid() const
        {
            return isSafeRelative(dirPathA) && isSafeRelative(dirPathB);
        }
    };
}  // namespace

helpers::CoreResult core::Diff::diffFiles(const std::string &filePathA,
                                          const std::string &filePathB)
{
    if (filePathA.empty() || filePathB.empty()) {
        return makeError("filePathA and filePathB cannot be empty");
    }

    std::string fullPathA = getBaseDir() + filePathA;
    std::string fullPathB = getBaseDir() + filePathB;

    std::cout << "diff api called" << std::endl;
    std::cout << "filePathA: " << fullPathA << std::endl;
    std::cout << "filePathB: " << fullPathB << std::endl;

    auto resultA = services::FileService::readFile(fullPathA);
    if (!resultA.success) {
        return makeError("Failed to read file A",
                         resultA.errorMessage + " (path: " + filePathA + ")");
    }

    auto resultB = services::FileService::readFile(fullPathB);
    if (!resultB.success) {
        return makeError("Failed to read file B",
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
        return makeError("Failed to perform diff", diffResult.errorMessage);
    }

    std::cout << "Successfully performed diff between " << nameA << " and " << nameB << std::endl;

    return makeSuccess(diffResult.data);
}

helpers::CoreResult core::Diff::diffWorkspaces(const std::string &dirPathA,
                                               const std::string &dirPathB)
{
    if (dirPathA.empty() || dirPathB.empty()) {
        return makeError("dirPathA and dirPathB cannot be empty");
    }

    // Validate paths
    if (!WorkspaceRequest::isSafeRelative(dirPathA) ||
        !WorkspaceRequest::isSafeRelative(dirPathB)) {
        return makeError("Invalid or unsafe path");
    }

    // ---- 경로 조립 & canonical ----
    fs::path joinedA = fs::path(getBaseDir()) / dirPathA;
    fs::path joinedB = fs::path(getBaseDir()) / dirPathB;

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
    auto ensureDir = [](const fs::path &p, const char *which) -> std::string {
        std::error_code e;
        if (!fs::exists(p, e)) {
            return std::string(which) + " not found: " + p.generic_string();
        }
        if (!fs::is_directory(p, e)) {
            return std::string(which) + " is not a directory: " + p.generic_string();
        }
        return "";
    };

    std::string errorA = ensureDir(fullA, "dirA");
    if (!errorA.empty()) {
        return makeError(errorA);
    }

    std::string errorB = ensureDir(fullB, "dirB");
    if (!errorB.empty()) {
        return makeError(errorB);
    }

    auto diffResult =
            services::DiffService::diffDirectories(fullA.generic_string(), fullB.generic_string());
    if (!diffResult.success) {
        return makeError("Failed to perform workspace diff", diffResult.errorMessage);
    }

    std::cout << "Successfully performed workspace diff between " << fullA.generic_string()
              << " and " << fullB.generic_string() << std::endl;

    return makeSuccess(diffResult.data);
}
