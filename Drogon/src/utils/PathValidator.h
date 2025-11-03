#pragma once

#include "logging/Logger.h"

#include <filesystem>
#include <string>

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

    inline bool validatePathWithinBase(const std::string &path, const std::string &baseDir)
    {
        if (!validatePath(path)) {
            return false;
        }

        try {
            namespace fs = std::filesystem;
            auto absPath = fs::absolute(path);
            auto absBase = fs::absolute(baseDir);

            // Check if path starts with base directory
            auto pathStr = absPath.string();
            auto baseStr = absBase.string();

            return pathStr.compare(0, baseStr.length(), baseStr) == 0;
        } catch (const std::exception &e) {
            utils::logging::error("Path validation error: " + std::string(e.what()));
            return false;
        }
    }

    inline bool isValidFile(const std::string &path)
    {
        try {
            namespace fs = std::filesystem;
            return fs::exists(path) && fs::is_regular_file(path);
        } catch (const std::exception &e) {
            utils::logging::error("File validation error: " + std::string(e.what()));
            return false;
        }
    }

    /**
     * @brief Check if path exists and is a directory
     * @param path Path to check
     * @return true if path exists and is a directory
     */
    inline bool isValidDirectory(const std::string &path)
    {
        try {
            namespace fs = std::filesystem;
            return fs::exists(path) && fs::is_directory(path);
        } catch (const std::exception &e) {
            utils::logging::error("Directory validation error: " + std::string(e.what()));
            return false;
        }
    }

}  // namespace utils
