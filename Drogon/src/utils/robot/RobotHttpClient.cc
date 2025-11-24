#include "RobotHttpClient.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void utils::RobotHttpClient::checkRunning(const std::string                             &scheme,
                                          const std::string                             &host,
                                          int                                            port,
                                          std::function<void(bool, const std::string &)> callback)
{
    std::string url = scheme + "://" + host + ":" + std::to_string(port);
    auto        client = drogon::HttpClient::newHttpClient(url);
    auto req    = drogon::HttpRequest::newHttpRequest();
    req->setPath(RUNNING_ENDPOINT);
    req->setMethod(drogon::Get);

    client->sendRequest(
            req, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                if (result != drogon::ReqResult::Ok) {
                    return callback(false, "Failed to connect to robot");
                }

                auto json = response->getJsonObject();
                if (!json || !json->isMember("data")) {
                    return callback(false, "Invalid response from robot");
                }

                bool isRunning = (*json)["data"].asBool();
                callback(isRunning, "");
            });
}

void utils::RobotHttpClient::uploadWorkspace(
        const std::string                             &scheme,
        const std::string                             &host,
        int                                            port,
        const std::string                             &filePath,
        std::function<void(bool, const std::string &)> callback)
{
    // Read file content
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return callback(false, "Failed to read file: " + filePath);
    }

    std::ostringstream fileBuffer;
    fileBuffer << file.rdbuf();
    std::string fileContent = fileBuffer.str();
    file.close();

    // Build multipart/form-data body
    std::string        boundary = "----WebKitFormBoundary" + drogon::utils::getUuid();
    std::ostringstream multipartBody;

    multipartBody << "--" << boundary << "\r\n";
    multipartBody << "Content-Disposition: form-data; name=\"file\"; "
                  << "filename=\"workspace.tar.gz\"\r\n";
    multipartBody << "Content-Type: application/gzip\r\n\r\n";
    multipartBody << fileContent;
    multipartBody << "\r\n--" << boundary << "--\r\n";

    // Create HTTP request
    std::string url    = scheme + "://" + host + ":" + std::to_string(port);
    auto        client = drogon::HttpClient::newHttpClient(url);
    auto req    = drogon::HttpRequest::newHttpRequest();
    req->setPath(IMPORT_ENDPOINT);
    req->setMethod(drogon::Post);
    req->setContentTypeCode(drogon::CT_MULTIPART_FORM_DATA);
    req->addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    req->setBody(multipartBody.str());

    client->sendRequest(
            req, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                if (result != drogon::ReqResult::Ok) {
                    return callback(false, "Failed to upload to robot");
                }

                auto json = response->getJsonObject();
                if (!json || !json->isMember("success")) {
                    return callback(false, "Invalid response from robot");
                }

                bool success = (*json)["success"].asBool();
                if (!success) {
                    std::string errorMsg = "Upload rejected by robot";
                    if (json->isMember("message")) {
                        errorMsg = (*json)["message"].asString();
                    }
                    return callback(false, errorMsg);
                }

                callback(true, "");
            });
}

void utils::RobotHttpClient::downloadWorkspace(
        const std::string                                            &scheme,
        const std::string                                            &host,
        int                                                           port,
        std::function<void(const std::string &, const std::string &)> callback)
{
    std::string url    = scheme + "://" + host + ":" + std::to_string(port);
    auto        client = drogon::HttpClient::newHttpClient(url);
    auto req    = drogon::HttpRequest::newHttpRequest();
    req->setPath(EXPORT_ENDPOINT);
    req->setMethod(drogon::Get);

    client->sendRequest(
            req, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                if (result != drogon::ReqResult::Ok) {
                    return callback("", "Failed to download from robot");
                }

                std::string content(response->getBody());
                if (content.empty()) {
                    return callback("", "Empty response from robot");
                }

                callback(content, "");
            });
}
