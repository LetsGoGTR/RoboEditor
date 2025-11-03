#include "FileService.h"

#include "../utils/PathValidator.h"
#include "../utils/logging/Logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

Json::Value services::FileService::getFileInfo(const std::string &filePath)
{
    Json::Value info;

    if (!utils::isValidFile(filePath)) {
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

    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    if (!utils::isValidFile(filePath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + filePath);
        return result;
    }

    try {
        std::ifstream     file(filePath, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();

        result.data["content"] = buffer.str();
        result.data["info"]    = getFileInfo(filePath);
        result.success         = true;

        utils::logging::info("Successfully read file: " + filePath);

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

    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    if (utils::isValidFile(filePath)) {
        result.errorMessage = "File already exists";
        utils::logging::warn("File already exists: " + filePath);
        return result;
    }

    try {
        // Create parent directories if they don't exist
        fs::path path(filePath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        std::ofstream file(filePath, std::ios::binary);
        if (!file) {
            result.errorMessage = "Failed to create file";
            utils::logging::error("Failed to create file: " + filePath);
            return result;
        }

        file << content;
        file.close();

        result.data["info"] = getFileInfo(filePath);
        result.success      = true;

        utils::logging::info("Successfully created file: " + filePath);

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

    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    if (!utils::isValidFile(filePath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + filePath);
        return result;
    }

    try {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file) {
            result.errorMessage = "Failed to open file for writing";
            utils::logging::error("Failed to open file: " + filePath);
            return result;
        }

        file << content;
        file.close();

        result.data["info"] = getFileInfo(filePath);
        result.success      = true;

        utils::logging::info("Successfully updated file: " + filePath);

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

    if (!utils::validatePath(filePath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path: " + filePath);
        return result;
    }

    if (!utils::isValidFile(filePath)) {
        result.errorMessage = "File does not exist";
        utils::logging::warn("File does not exist: " + filePath);
        return result;
    }

    try {
        fs::remove(filePath);
        result.success = true;

        utils::logging::info("Successfully deleted file: " + filePath);

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

    if (!utils::validatePath(oldPath) || !utils::validatePath(newPath)) {
        result.errorMessage = "Invalid file path";
        utils::logging::warn("Invalid file path - old: " + oldPath + ", new: " + newPath);
        return result;
    }

    if (!utils::isValidFile(oldPath)) {
        result.errorMessage = "Source file does not exist";
        utils::logging::warn("Source file does not exist: " + oldPath);
        return result;
    }

    if (utils::isValidFile(newPath)) {
        result.errorMessage = "Destination file already exists";
        utils::logging::warn("Destination file already exists: " + newPath);
        return result;
    }

    try {
        // Create parent directories if they don't exist
        fs::path path(newPath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        // Move/rename the file
        fs::rename(oldPath, newPath);

        result.data["oldPath"] = oldPath;
        result.data["newPath"] = newPath;
        result.data["info"]    = getFileInfo(newPath);
        result.success         = true;

        utils::logging::info("Successfully moved file from: " + oldPath + " to: " + newPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to move file: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}
