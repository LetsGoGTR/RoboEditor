#include "ValidationController.h"

#include "../utils/connection/ConnectionValidator.h"
#include "../utils/device/DeviceMetadataHelper.h"
#include "ControllerHelper.h"

using helpers::sendError;

void api::v1::Validation::device(
        const drogon::HttpRequestPtr                          &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json) {
        return sendError(callback, drogon::k400BadRequest, "Invalid JSON body");
    }

    // Extract connection info from body
    if (!json->isMember("ip")) {
        return sendError(callback, drogon::k400BadRequest, "Missing required field: ip");
    }

    try {
        // Build connection params from body
        utils::ConnectionParams params;
        params.sftpHost     = (*json)["ip"].asString();
        params.sftpPort     = json->isMember("sftpPort") ? (*json)["sftpPort"].asInt() : 22;
        params.sftpUser     = json->isMember("sftpUser") ? (*json)["sftpUser"].asString() : "";
        params.sftpPassword = json->isMember("sftpPassword") ? (*json)["sftpPassword"].asString() : "";

        int apiPort   = json->isMember("apiPort") ? (*json)["apiPort"].asInt() : 80;
        params.apiUrl = "http://" + params.sftpHost + ":" + std::to_string(apiPort);

        // Validate connections
        auto validationResult = utils::ConnectionValidator::validateDeviceFromParams(params);

        Json::Value response;
        response["success"] = true;
        response["data"]    = validationResult.toJson();

        if (validationResult.isFullyValid()) {
            response["message"] = "Connection validation successful";
        } else {
            if (!validationResult.sftpValid && !validationResult.apiValid) {
                response["message"] = "Both SFTP and API connections failed";
            } else if (!validationResult.sftpValid) {
                response["message"] = "SFTP connection failed";
            } else {
                response["message"] = "API connection failed";
            }
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

        LOG_INFO << "Validated connection to " << params.sftpHost << " (SFTP: "
                 << (validationResult.sftpValid ? "OK" : "FAIL")
                 << ", API: " << (validationResult.apiValid ? "OK" : "FAIL") << ")";

    } catch (const std::exception &e) {
        LOG_ERROR << "ValidateConnection exception: " << e.what();
        sendError(callback, drogon::k500InternalServerError, "Internal server error", e.what());
    }
}
