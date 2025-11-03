#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>

#include "../utils/ConfigUtils.h"
#include "../utils/logging/Logger.h"

std::string services::AuthService::getPasswordHashFromConfig()
{
    return utils::config::getPasswordHash();
}

bool services::AuthService::verifyDevicePassword(const std::string &password)
{
    if (password.empty()) {
        utils::logging::warn("Empty password provided for verification");
        return false;
    }

    std::string storedHash = getPasswordHashFromConfig();

    if (storedHash.empty()) {
        utils::logging::error("No password hash configured - authentication will fail");
        return false;
    }

    try {
        bool isValid = BCrypt::validatePassword(password, storedHash);

        if (isValid) {
            utils::logging::info("Device password verification successful");
        } else {
            utils::logging::warn("Device password verification failed - invalid password");
        }

        return isValid;

    } catch (const std::exception &e) {
        utils::logging::error("Bcrypt verification exception: " + std::string(e.what()));
        return false;
    }
}
