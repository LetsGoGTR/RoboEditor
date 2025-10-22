#include "YamlFileController.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

#include "../models/FileRequest.h"

using namespace api::v1;
using namespace drogon_model;
namespace fs = std::filesystem;

void Yaml::diff(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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

    LOG_DEBUG << "diff api called";
    LOG_DEBUG << "filePathA: " << fileReq->filePathA;
    LOG_DEBUG << "filePathB: " << fileReq->filePathB;

    // 파일 확인
    if (!fs::exists(fileReq->filePathA)) {
        Json::Value error;
        error["error"] = "filePathA not found";
        error["path"]  = fileReq->filePathA;
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    if (!fs::exists(fileReq->filePathB)) {
        Json::Value error;
        error["error"] = "filePathB not found";
        error["path"]  = fileReq->filePathB;
        auto resp      = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }

    try {
        // YAML 파일 읽기
        std::ifstream fileA(fileReq->filePathA);
        std::ifstream fileB(fileReq->filePathB);

        std::stringstream bufferA, bufferB;
        bufferA << fileA.rdbuf();
        bufferB << fileB.rdbuf();

        std::string yamlContentA = bufferA.str();
        std::string yamlContentB = bufferB.str();

        fileA.close();
        fileB.close();

        // YAML 파싱 검증
        try {
            YAML::Load(yamlContentA);
        } catch (const YAML::Exception &e) {
            Json::Value error;
            error["error"]   = "Invalid YAML format in filePathA";
            error["message"] = e.what();
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        try {
            YAML::Load(yamlContentB);
        } catch (const YAML::Exception &e) {
            Json::Value error;
            error["error"]   = "Invalid YAML format in filePathB";
            error["message"] = e.what();
            auto resp        = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        // 응답 생성
        Json::Value ret;
        ret["yamlA"]      = yamlContentA;
        ret["yamlB"]      = yamlContentB;
        ret["diffResult"] = "";  // 추후 diff 구현

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);

        LOG_INFO << "Successfully read YAML files";

    } catch (const std::exception &e) {
        LOG_ERROR << "Error reading YAML files: " << e.what();

        Json::Value error;
        error["error"]   = "Failed to read YAML files";
        error["message"] = e.what();
        auto resp        = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
