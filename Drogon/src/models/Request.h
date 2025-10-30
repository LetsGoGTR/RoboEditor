#pragma once

#include <drogon/drogon.h>
#include <optional>
#include <string>

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

        static bool isSafeRelative(const std::string& p)
        {
            if (p.empty()) return false;
            // 절대경로 형태 차단 (리눅스/맥: '/', 윈도우: 드라이브 + ':', 혹은 '\\' 시작)
            if (p.size() >= 1 && (p[0] == '/' || p[0] == '\\')) return false;
            if (p.size() >= 2 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':') return false;
            // 상위 폴더 탈출 방지
            if (p.find("..") != std::string::npos) return false;
            return true;
        }

        bool isValid() const
        {
            return isSafeRelative(dirPathA) && isSafeRelative(dirPathB);
        }

        Json::Value toJson() const
        {
            Json::Value json;
            json["dirPathA"] = dirPathA;
            json["dirPathB"] = dirPathB;
            return json;
        }
    };
}  // namespace drogon_model
