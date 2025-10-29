#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>
#include <drogon/drogon.h>

std::string services::AuthService::getPasswordHashFromConfig()
{
    try {
        const Json::Value &customConfig = drogon::app().getCustomConfig();

        if (!customConfig.isMember("auth") || !customConfig["auth"].isMember("device_password_hash")) {
            LOG_ERROR << "Missing auth.device_password_hash in config.json custom_config";
            return "";
        }

        std::string hash = customConfig["auth"]["device_password_hash"].asString();

        if (hash.empty()) {
            LOG_ERROR << "device_password_hash is empty in config.json";
            return "";
        }

        return hash;

    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to load password hash from config: " << e.what();
        return "";
    }
}

bool services::AuthService::verifyDevicePassword(const std::string &password)
{
    if (password.empty()) {
        LOG_WARN << "Empty password provided for verification";
        return false;
    }

    std::string storedHash = getPasswordHashFromConfig();

    if (storedHash.empty()) {
        LOG_ERROR << "No password hash configured - authentication will fail";
        return false;
    }

    try {
        bool isValid = BCrypt::validatePassword(password, storedHash);

        if (isValid) {
            LOG_INFO << "Device password verification successful";
        } else {
            LOG_WARN << "Device password verification failed - invalid password";
        }

        return isValid;

    } catch (const std::exception &e) {
        LOG_ERROR << "Bcrypt verification exception: " << e.what();
        return false;
    }
}
