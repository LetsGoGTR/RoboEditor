#pragma once

#include <json/json.h>
#include <string>
#include <drogon/drogon.h>

namespace services
{
    struct ServiceResult
    {
        bool        success;
        std::string errorMessage;
        Json::Value data;

        // Constructors for convenience
        ServiceResult() : success(false), errorMessage(""), data(Json::objectValue) {}

        // Success factory method with optional logging
        static ServiceResult createSuccess(const Json::Value &resultData = Json::objectValue,
                                           const std::string &logMessage = "")
        {
            ServiceResult result;
            result.success = true;
            result.data    = resultData;

            if (!logMessage.empty()) {
                LOG_INFO << logMessage;
            }

            return result;
        }

        // Error factory method with automatic logging
        static ServiceResult createError(const std::string &message, bool autoLog = true)
        {
            ServiceResult result;
            result.success      = false;
            result.errorMessage = message;

            if (autoLog) {
                LOG_ERROR << message;
            }

            return result;
        }
    };

}  // namespace services
