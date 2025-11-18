#include "FolderService.h"

#include <filesystem>

#include "../utils/ConfigUtils.h"
#include "../utils/PathValidator.h"
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

Json::Value services::FolderService::getFolderInfo(const std::string &folderPath)
{
    Json::Value info;

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return info;
    }

    try {
        fs::path path(folderPath);

        info["path"] = folderPath;
        info["name"] = path.filename().string();

        // Count files and subdirectories
        int fileCount = 0;
        int dirCount  = 0;

        for (const auto &entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                fileCount++;
            } else if (entry.is_directory()) {
                dirCount++;
            }
        }

        info["fileCount"]      = fileCount;
        info["directoryCount"] = dirCount;

    } catch (const std::exception &e) {
        utils::logging::error("Error getting folder info: " + std::string(e.what()));
    }

    return info;
}

services::ServiceResult services::FolderService::createFolder(const std::string &folderPath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + folderPath;

    // Check if folder already exists
    if (fs::exists(fullPath) && fs::is_directory(fullPath)) {
        result.errorMessage = "Folder already exists";
        utils::logging::warn("Folder already exists: " + fullPath);
        return result;
    }

    try {
        fs::create_directories(fullPath);
        result.data["info"] = getFolderInfo(fullPath);
        result.success      = true;

        utils::logging::info("Successfully created folder: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to create folder: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FolderService::updateFolder(const std::string &oldPath,
                                                              const std::string &newPath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative paths for security
    if (!utils::validatePath(oldPath) || !utils::validatePath(newPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + oldPath + " or " + newPath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullOldPath = utils::config::getBaseDir() + oldPath;
    std::string fullNewPath = utils::config::getBaseDir() + newPath;

    // Check if source folder exists
    if (!fs::exists(fullOldPath) || !fs::is_directory(fullOldPath)) {
        result.errorMessage = "Source folder does not exist";
        utils::logging::warn("Source folder does not exist: " + fullOldPath);
        return result;
    }

    // Check if destination already exists
    if (fs::exists(fullNewPath) && fs::is_directory(fullNewPath)) {
        result.errorMessage = "Destination folder already exists";
        utils::logging::warn("Destination folder already exists: " + fullNewPath);
        return result;
    }

    try {
        fs::rename(fullOldPath, fullNewPath);
        result.data["info"] = getFolderInfo(fullNewPath);
        result.success      = true;

        utils::logging::info("Successfully renamed folder from " + fullOldPath + " to " +
                             fullNewPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to rename folder: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FolderService::deleteFolder(const std::string &folderPath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + folderPath;

    // Check if folder exists
    if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
        result.errorMessage = "Folder does not exist";
        utils::logging::warn("Folder does not exist: " + fullPath);
        return result;
    }

    try {
        fs::remove_all(fullPath);
        result.success = true;

        utils::logging::info("Successfully deleted folder: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to delete folder: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}

services::ServiceResult services::FolderService::readFolder(const std::string &folderPath)
{
    services::ServiceResult result;
    result.success = false;

    // Validate relative path for security
    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    // Combine with base directory after validation
    std::string fullPath = utils::config::getBaseDir() + folderPath;

    // Check if folder exists
    if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
        result.errorMessage = "Folder does not exist";
        utils::logging::warn("Folder does not exist: " + fullPath);
        return result;
    }

    try {
        Json::Value files(Json::arrayValue);
        Json::Value directories(Json::arrayValue);

        for (const auto &entry : fs::directory_iterator(fullPath)) {
            Json::Value item;
            item["name"] = entry.path().filename().string();
            item["path"] = entry.path().string();

            if (entry.is_regular_file()) {
                item["type"] = "file";
                item["size"] = (Json::Int64)entry.file_size();
                files.append(item);
            } else if (entry.is_directory()) {
                item["type"] = "directory";
                directories.append(item);
            }
        }

        result.data["files"]       = files;
        result.data["directories"] = directories;
        result.data["info"]        = getFolderInfo(fullPath);
        result.success             = true;

        utils::logging::info("Successfully listed folder: " + fullPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to list folder: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}
