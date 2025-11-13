#pragma once

#include <string>

#include "logging/Logger.h"

namespace utils
{
    inline bool validatePath(const std::string &path)
    {
        if (path.empty()) {
            return false;
        }

        // Check for path traversal attacks
        if (path.find("..") != std::string::npos) {
            return false;
        }

        return true;
    }

}  // namespace utils
