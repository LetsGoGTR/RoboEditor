#include "FileController.h"

#include <filesystem>

#include "../services/FileService.h"

using namespace api::v1;
using namespace services;
namespace fs = std::filesystem;

void File::upload(const HttpRequestPtr                          &req,
                  std::function<void(const HttpResponsePtr &)> &&callback)
{
    MultiPartParser fileUpload;
    fileUpload.parse(req);

    auto files = fileUpload.getFiles();
    if (files.empty()) {
        Json::Value error;
        error["error"] = "No file uploaded";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto       &file             = files[0];
    std::string originalFilename = file.getFileName();

    // 압축 파일 형식 확인
    if (!FileService::isSupportedArchive(originalFilename)) {
        Json::Value error;
        error["error"] = "Unsupported file format. Supported: .zip, .tar.gz, .7z, etc.";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    // 고유 ID 생성
    std::string workspaceId = drogon::utils::getUuid();

    // 임시 저장 경로
    std::string tempDir = "/tmp/drogon-app/temp";
    FileService::createDirectory(tempDir);
    std::string tempFilePath = tempDir + "/" + workspaceId + "_" + originalFilename;

    // 압축 해제 경로
    std::string workspacePath = "/tmp/drogon-app/storage/" + workspaceId;

    try {
        // 업로드된 파일 임시 저장
        file.saveAs(tempFilePath);
        LOG_INFO << "File saved to: " << tempFilePath;

        // 압축 해제
        auto result = FileService::extractArchive(tempFilePath, workspacePath);

        // 임시 파일 삭제
        FileService::removeFile(tempFilePath);

        if (!result.success) {
            Json::Value error;
            error["error"]   = "Extraction failed";
            error["message"] = result.errorMessage;
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }

        // 성공 응답
        Json::Value ret;
        ret["success"]           = true;
        ret["workspace_id"]      = workspaceId;
        ret["workspace_path"]    = workspacePath;
        ret["original_filename"] = originalFilename;

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k201Created);
        callback(resp);

        LOG_INFO << "Extraction completed: " << workspaceId;

    } catch (const std::exception &e) {
        LOG_ERROR << "Exception during extraction: " << e.what();

        FileService::removeFile(tempFilePath);
        FileService::removeDirectory(workspacePath);

        Json::Value error;
        error["error"]   = "Internal server error";
        error["message"] = e.what();
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void File::listWorkspace(const HttpRequestPtr                          &req,
                         std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string storageRoot = "/tmp/drogon-app/storage";

    // 디렉토리 확인
    if (!fs::exists(storageRoot)) {
        FileService::createDirectory(storageRoot);
    }

    Json::Value response;
    response["success"] = true;

    Json::Value workspcaes(Json::arrayValue);

    try {
        for (const auto &entry : fs::directory_iterator(storageRoot)) {
            if (entry.is_directory()) {
                std::string workspcaeId = entry.path().filename().string();
                workspcaes.append(workspcaeId);
            }
        }

        response["workspcaes"] = workspcaes;
        response["count"]      = (int)workspcaes.size();

        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);

        LOG_INFO << "Listed " << workspcaes.size() << " workspace IDs";

    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to list workspaces: " << e.what();
        Json::Value error;
        error["error"] = "Failed to list extracts";
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
