#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>

#include "../utils/ConfigUtils.h"

std::string services::AuthService::getPasswordHashFromConfig()
{
    return utils::config::getPasswordHash();
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
