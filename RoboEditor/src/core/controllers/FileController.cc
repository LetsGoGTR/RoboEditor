#include "FileController.h"

#include <iostream>

#include "../services/FileService.h"
#include "../utils/ConfigUtils.h"

using helpers::makeError;
using helpers::makeSuccess;
using utils::config::getBaseDir;

helpers::CoreResult core::File::readFile(const std::string &path)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FileService::readFile(fullPath);

    if (!result.success) {
        return makeError("Failed to read file", result.errorMessage);
    }

    std::cout << "File read: " << fullPath << std::endl;

    return makeSuccess(result.data);
}

helpers::CoreResult core::File::createFile(const std::string &path, const std::string &content)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FileService::createFile(fullPath, content);

    if (!result.success) {
        return makeError("Failed to create file", result.errorMessage);
    }

    std::cout << "File created: " << fullPath << std::endl;

    return makeSuccess(result.data, "File created successfully");
}

helpers::CoreResult core::File::updateFile(const std::string &path,
                                           const std::string &content,
                                           const std::string &newPath)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    bool hasContent = !content.empty();
    bool hasNewPath = !newPath.empty();

    if (!hasContent && !hasNewPath) {
        return makeError("Either 'content' or 'newPath' must be provided");
    }

    std::string currentPath = fullPath;

    // Move file if newPath is provided
    if (hasNewPath) {
        std::string fullNewPath = getBaseDir() + newPath;
        auto        result      = services::FileService::moveFile(currentPath, fullNewPath);

        if (!result.success) {
            return makeError("Failed to move file", result.errorMessage);
        }

        currentPath = fullNewPath;  // Update path for content update
    }

    // Update content if provided
    if (hasContent) {
        auto result = services::FileService::updateFile(currentPath, content);

        if (!result.success) {
            return makeError("Failed to update file content", result.errorMessage);
        }
    }

    // Success response
    auto        result = services::FileService::getFileInfo(currentPath);
    Json::Value data;
    data["path"] = currentPath;
    data["info"] = result;

    std::cout << "File updated: " << fullPath << (hasNewPath ? " -> " + currentPath : "")
              << std::endl;

    return makeSuccess(data, "File updated successfully");
}

helpers::CoreResult core::File::deleteFile(const std::string &path)
{
    std::string fullPath = getBaseDir() + path;

    if (path.empty()) {
        return makeError("Missing 'path' parameter");
    }

    auto result = services::FileService::deleteFile(fullPath);

    if (!result.success) {
        return makeError("Failed to delete file", result.errorMessage);
    }

    Json::Value data;
    std::cout << "File deleted: " << fullPath << std::endl;

    return makeSuccess(data, "File deleted successfully");
}
