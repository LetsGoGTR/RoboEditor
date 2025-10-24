#include "FolderService.h"

#include <filesystem>

namespace fs = std::filesystem;

bool services::FolderService::validateFolderPath(const std::string &folderPath)
{
    if (folderPath.empty()) {
        return false;
    }

    // Check for path traversal attacks
    if (folderPath.find("..") != std::string::npos) {
        return false;
    }

    return true;
}

bool services::FolderService::folderExists(const std::string &folderPath)
{
    try {
        return std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath);
    } catch (const std::exception &e) {
        LOG_ERROR << "Error checking folder existence: " << e.what();
        return false;
    }
}

Json::Value services::FolderService::getFolderInfo(const std::string &folderPath)
{
    Json::Value info;

    if (!folderExists(folderPath)) {
        return info;
    }

    try {
        std::filesystem::path path(folderPath);

        info["path"] = folderPath;
        info["name"] = path.filename().string();

        // Count files and subdirectories
        int fileCount = 0;
        int dirCount  = 0;

        for (const auto &entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                fileCount++;
            } else if (entry.is_directory()) {
                dirCount++;
            }
        }

        info["fileCount"]      = fileCount;
        info["directoryCount"] = dirCount;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error getting folder info: " << e.what();
    }

    return info;
}

services::FolderOperationResult services::FolderService::createFolder(const std::string &folderPath)
{
    services::FolderOperationResult result;
    result.success = false;

    if (!validateFolderPath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        LOG_WARN << "Invalid folder path: " << folderPath;
        return result;
    }

    if (folderExists(folderPath)) {
        result.errorMessage = "Folder already exists";
        LOG_WARN << "Folder already exists: " << folderPath;
        return result;
    }

    try {
        std::filesystem::create_directories(folderPath);
        result.data["info"] = getFolderInfo(folderPath);
        result.success      = true;

        LOG_INFO << "Successfully created folder: " << folderPath;

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to create folder: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}

services::FolderOperationResult services::FolderService::updateFolder(const std::string &oldPath,
                                                                      const std::string &newPath)
{
    services::FolderOperationResult result;
    result.success = false;

    if (!validateFolderPath(oldPath) || !validateFolderPath(newPath)) {
        result.errorMessage = "Invalid folder path";
        LOG_WARN << "Invalid folder path: " << oldPath << " or " << newPath;
        return result;
    }

    if (!folderExists(oldPath)) {
        result.errorMessage = "Source folder does not exist";
        LOG_WARN << "Source folder does not exist: " << oldPath;
        return result;
    }

    if (folderExists(newPath)) {
        result.errorMessage = "Destination folder already exists";
        LOG_WARN << "Destination folder already exists: " << newPath;
        return result;
    }

    try {
        std::filesystem::rename(oldPath, newPath);
        result.data["info"] = getFolderInfo(newPath);
        result.success      = true;

        LOG_INFO << "Successfully renamed folder from " << oldPath << " to " << newPath;

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to rename folder: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}

services::FolderOperationResult services::FolderService::deleteFolder(const std::string &folderPath)
{
    services::FolderOperationResult result;
    result.success = false;

    if (!validateFolderPath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        LOG_WARN << "Invalid folder path: " << folderPath;
        return result;
    }

    if (!folderExists(folderPath)) {
        result.errorMessage = "Folder does not exist";
        LOG_WARN << "Folder does not exist: " << folderPath;
        return result;
    }

    try {
        std::filesystem::remove_all(folderPath);
        result.success = true;

        LOG_INFO << "Successfully deleted folder: " << folderPath;

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to delete folder: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}

services::FolderOperationResult services::FolderService::readFolder(const std::string &folderPath)
{
    services::FolderOperationResult result;
    result.success = false;

    if (!validateFolderPath(folderPath)) {
        result.errorMessage = "Invalid folder path";
        LOG_WARN << "Invalid folder path: " << folderPath;
        return result;
    }

    if (!folderExists(folderPath)) {
        result.errorMessage = "Folder does not exist";
        LOG_WARN << "Folder does not exist: " << folderPath;
        return result;
    }

    try {
        Json::Value files(Json::arrayValue);
        Json::Value directories(Json::arrayValue);

        for (const auto &entry : std::filesystem::directory_iterator(folderPath)) {
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

        LOG_INFO << "Successfully listed folder: " << folderPath;

    } catch (const std::exception &e) {
        result.errorMessage = "Failed to list folder: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}
