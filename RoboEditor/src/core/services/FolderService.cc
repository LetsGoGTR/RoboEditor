#include "FolderService.h"

#include <filesystem>

#include "../utils/PathValidator.h"
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

Json::Value services::FolderService::getFolderInfo(const std::string &folderPath)
{
    Json::Value info;

    if (!utils::isValidDirectory(folderPath)) {
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

    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    if (utils::isValidDirectory(folderPath)) {
        result.errorMessage = "Folder already exists";
        utils::logging::warn("Folder already exists: " + folderPath);
        return result;
    }

    try {
        fs::create_directories(folderPath);
        result.data["info"] = getFolderInfo(folderPath);
        result.success      = true;

        utils::logging::info("Successfully created folder: " + folderPath);

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

    if (!utils::validatePath(oldPath) || !utils::validatePath(newPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + oldPath + " or " + newPath);
        return result;
    }

    if (!utils::isValidDirectory(oldPath)) {
        result.errorMessage = "Source folder does not exist";
        utils::logging::warn("Source folder does not exist: " + oldPath);
        return result;
    }

    if (utils::isValidDirectory(newPath)) {
        result.errorMessage = "Destination folder already exists";
        utils::logging::warn("Destination folder already exists: " + newPath);
        return result;
    }

    try {
        fs::rename(oldPath, newPath);
        result.data["info"] = getFolderInfo(newPath);
        result.success      = true;

        utils::logging::info("Successfully renamed folder from " + oldPath + " to " + newPath);

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

    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    if (!utils::isValidDirectory(folderPath)) {
        result.errorMessage = "Folder does not exist";
        utils::logging::warn("Folder does not exist: " + folderPath);
        return result;
    }

    try {
        fs::remove_all(folderPath);
        result.success = true;

        utils::logging::info("Successfully deleted folder: " + folderPath);

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

    if (!utils::validatePath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        utils::logging::warn("Invalid folder path: " + folderPath);
        return result;
    }

    if (!utils::isValidDirectory(folderPath)) {
        result.errorMessage = "Folder does not exist";
        utils::logging::warn("Folder does not exist: " + folderPath);
        return result;
    }

    try {
        Json::Value files(Json::arrayValue);
        Json::Value directories(Json::arrayValue);

        for (const auto &entry : fs::directory_iterator(folderPath)) {
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
        result.data["info"]        = getFolderInfo(folderPath);
        result.success             = true;

        utils::logging::info("Successfully listed folder: " + folderPath);

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to list folder: " + std::string(e.what());
        utils::logging::error(result.errorMessage);
    }

    return result;
}
