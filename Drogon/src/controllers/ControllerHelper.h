#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <json/json.h>
#include <string>

namespace helpers
{
    inline void sendError(std::function<void(const drogon::HttpResponsePtr &)> &callback,
                          drogon::HttpStatusCode                                status,
                          const std::string                                    &error,
                          const std::string                                    &details = "")
    {
        Json::Value json;
        json["success"] = false;

        if (details.empty()) {
            json["message"] = error;
        } else {
            json["message"] = error + ": " + details;
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(status);
        callback(resp);
    }

    inline void sendSuccess(std::function<void(const drogon::HttpResponsePtr &)> &callback,
                            drogon::HttpStatusCode                                status,
                            const Json::Value                                    &data,
                            const std::string                                    &message = "")
    {
        Json::Value response;
        response["success"] = true;

        if (!message.empty()) {
            response["message"] = message;
        }

        response["data"] = data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(status);
        callback(resp);
    }

}  // namespace helpers
