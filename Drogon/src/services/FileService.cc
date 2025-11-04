#include "FileService.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "../utils/ConfigUtils.h"
#include "../utils/PathValidator.h"
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

Json::Value services::FileService::getFileInfo(const std::string &filePath)
{
    Json::Value info;

    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
        return info;
    }

    try {
        fs::path path(filePath);

        info["path"]      = filePath;
        info["name"]      = path.filename().string();
        info["extension"] = path.extension().string();
        info["size"]      = (Json::Int64)fs::file_size(filePath);

        auto ftime = fs::last_write_time(filePath);
        auto sctp  = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        auto time_t_val  = std::chrono::system_clock::to_time_t(sctp);
        info["modified"] = (Json::Int64)time_t_val;

    } catch (const std::exception &e) {
        utils::logging::error("Error getting file info: " + std::string(e.what()));
    }

    return info;
}

services::ServiceResult services::FileService::readFile(const std::string &filePath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + filePath;

    // Check if file exists
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + fullPath);
        return result;
    }

    try {
        std::ifstream     file(fullPath, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();

        result.data["content"] = buffer.str();
        result.data["info"]    = getFileInfo(fullPath);
        result.success         = true;

        utils::logging::info("Successfully read file: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to read file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FileService::createFile(const std::string &filePath,
                                                          const std::string &content)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + filePath;

    // Check if file already exists
    if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
        result.errorMessage = "File already exists";
        utils::logging::warn("File already exists: " + fullPath);
        return result;
    }

    try {
        // Create parent directories if they don't exist
        fs::path path(fullPath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        std::ofstream file(fullPath, std::ios::binary);
        if (!file) {
            result.errorMessage = "Failed to create file";
            utils::logging::error("Failed to create file: " + fullPath);
            return result;
        }

        file << content;
        file.close();

        result.data["info"] = getFileInfo(fullPath);
        result.success      = true;

        utils::logging::info("Successfully created file: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to create file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FileService::updateFile(const std::string &filePath,
                                                          const std::string &content)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + filePath;

    // Check if file exists
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + fullPath);
        return result;
    }

    try {
        std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            result.errorMessage = "Failed to open file for writing";
            utils::logging::error("Failed to open file: " + fullPath);
            return result;
        }

        file << content;
        file.close();

        result.data["info"] = getFileInfo(fullPath);
        result.success      = true;

        utils::logging::info("Successfully updated file: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to update file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FileService::deleteFile(const std::string &filePath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + filePath;

    // Check if file exists
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + fullPath);
        return result;
    }

    try {
        fs::remove(fullPath);
        result.success = true;

        utils::logging::info("Successfully deleted file: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to delete file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FileService::moveFile(const std::string &oldPath,
                                                        const std::string &newPath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative paths for security
    if (!utils::validatePath(oldPath) || !utils::validatePath(newPath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path - old: " + oldPath + ", new: " + newPath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullOldPath = utils::config::getBaseDir() + oldPath;
    std::string fullNewPath = utils::config::getBaseDir() + newPath;

    // Check if source file exists
    if (!fs::exists(fullOldPath) || !fs::is_regular_file(fullOldPath)) {
        result.errorMessage = "Source file does not exist";
        utils::logging::warn("Source file does not exist: " + fullOldPath);
        return result;
    }

    // Check if destination already exists
    if (fs::exists(fullNewPath) && fs::is_regular_file(fullNewPath)) {
        result.errorMessage = "Destination file already exists";
        utils::logging::warn("Destination file already exists: " + fullNewPath);
        return result;
    }

    try {
        // Create parent directories if they don't exist
        fs::path path(fullNewPath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        // Move/rename the file
        fs::rename(fullOldPath, fullNewPath);

        result.data["oldPath"] = fullOldPath;
        result.data["newPath"] = fullNewPath;
        result.data["info"]    = getFileInfo(fullNewPath);
        result.success         = true;

        utils::logging::info("Successfully moved file from: " + fullOldPath +
                             " to: " + fullNewPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to move file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}
