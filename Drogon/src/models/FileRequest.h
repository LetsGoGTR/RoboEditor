#pragma once

#include <drogon/drogon.h>
#include <optional>
#include <string>

using namespace drogon;

namespace drogon_model
{

    struct FileRequest
    {
        std::string filePathA;
        std::string filePathB;

        static std::optional<FileRequest> fromJson(const Json::Value &json)
        {
            if (!json.isMember("filePathA") || !json.isMember("filePathB")) {
                return std::nullopt;
            }

            FileRequest req;
            req.filePathA = json["filePathA"].asString();
            req.filePathB = json["filePathB"].asString();
            return req;
        }

        bool isValid() const
        {
            return !filePathA.empty() && !filePathB.empty();
        }

        Json::Value toJson() const
        {
            Json::Value json;
            json["filePathA"] = filePathA;
            json["filePathB"] = filePathB;
            return json;
        }
    };

}  // namespace drogon_model
