#include "FolderController.h"

#include <iostream>

#include "../services/FolderService.h"
#include "../utils/ConfigUtils.h"

using helpers::makeError;
using helpers::makeSuccess;
using utils::config::getBaseDir;

helpers::CoreResult core::Folder::readFolder(const std::string &path)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FolderService::readFolder(fullPath);

    if (!result.success) {
        return makeError("Failed to list folder", result.errorMessage);
    }

    std::cout << "Folder listed: " << fullPath << std::endl;

    return makeSuccess(result.data);
}

helpers::CoreResult core::Folder::createFolder(const std::string &path)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FolderService::createFolder(fullPath);

    if (!result.success) {
        return makeError("Failed to create folder", result.errorMessage);
    }

    std::cout << "Folder created: " << fullPath << std::endl;

    return makeSuccess(result.data, "Folder created successfully");
}

helpers::CoreResult core::Folder::updateFolder(const std::string &path, const std::string &newPath)
{
    std::string oldPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    if (newPath.empty()) {
        return makeError("New path is required");
    }

    std::string fullNewPath = getBaseDir() + newPath;

    auto result = services::FolderService::updateFolder(oldPath, fullNewPath);

    if (!result.success) {
        return makeError("Failed to rename folder", result.errorMessage);
    }

    std::cout << "Folder renamed from: " << oldPath << " to: " << fullNewPath << std::endl;

    return makeSuccess(result.data, "Folder renamed successfully");
}

helpers::CoreResult core::Folder::deleteFolder(const std::string &path)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FolderService::deleteFolder(fullPath);

    if (!result.success) {
        return makeError("Failed to delete folder", result.errorMessage);
    }

    Json::Value data;
    std::cout << "Folder deleted: " << fullPath << std::endl;

    return makeSuccess(data, "Folder deleted successfully");
}
