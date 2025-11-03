#pragma once

#include <json/json.h>
#include <string>

namespace helpers
{
    // Core 결과를 담는 구조체
    struct CoreResult
    {
        bool        success;
        Json::Value data;
        std::string message;

        CoreResult(bool s, const Json::Value &d, const std::string &msg) :
            success(s),
            data(d),
            message(msg)
        {
        }
    };

    // 에러 결과 생성 헬퍼 함수
    inline CoreResult makeError(const std::string &error, const std::string &details = "")
    {
        Json::Value emptyData;
        std::string fullMessage = details.empty() ? error : (error + ": " + details);
        return CoreResult(false, emptyData, fullMessage);
    }

    // 성공 결과 생성 헬퍼 함수
    inline CoreResult makeSuccess(const Json::Value &data, const std::string &message = "")
    {
        return CoreResult(true, data, message);
    }

}  // namespace helpers
