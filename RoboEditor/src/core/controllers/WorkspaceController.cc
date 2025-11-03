#include "WorkspaceController.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "../services/WorkspaceService.h"
#include "../utils/ConfigUtils.h"
#include "../utils/TimeUtils.h"

namespace fs = std::filesystem;

using helpers::makeError;
using helpers::makeSuccess;
using utils::config::getBaseDir;
using utils::config::getTempExportDir;
using utils::config::getTempUploadDir;

// 간단한 UUID 생성 함수 (DeviceController와 동일)
static std::string generateUuid()
{
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex << ms.count();
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }

    return ss.str();
}

helpers::CoreResult core::Workspace::importWorkspace(const std::string &filePath,
                                                     const std::string &name,
                                                     const std::string &target,
                                                     const std::string &description)
{
    if (!fs::exists(filePath)) {
        return makeError("File not found", filePath);
    }

    std::string filename = fs::path(filePath).filename().string();

    if (!services::WorkspaceService::isSupportedArchive(filename)) {
        return makeError("Unsupported file format");
    }

    // Build metadata
    std::string workspaceName = name.empty() ? fs::path(filename).stem().string() : name;

    services::WorkspaceMetadata wsMetadata;
    wsMetadata.id          = generateUuid();
    wsMetadata.target      = target;
    wsMetadata.name        = workspaceName;
    wsMetadata.description = description;
    wsMetadata.createdAt   = utils::getCurrentTimestamp();
    wsMetadata.updatedAt   = wsMetadata.createdAt;

    try {
        auto result =
                services::WorkspaceService::importWorkspace(filePath, getBaseDir(), wsMetadata);

        if (!result.success) {
            return makeError("Failed to import workspace", result.errorMessage);
        }

        std::cout << "Imported: " << workspaceName << " (ID: " << wsMetadata.id << ")" << std::endl;

        return makeSuccess(result.data, "Workspace imported successfully");

    } catch (const std::exception &e) {
        std::cerr << "Import exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Workspace::exportWorkspace(const std::string &workspaceId)
{
    std::string workspacePath = getBaseDir() + workspaceId;

    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        return makeError("Workspace not found", workspaceId);
    }

    try {
        auto        metadata      = services::WorkspaceService::loadMetadata(workspacePath);
        std::string workspaceName = metadata.name.empty() ? workspaceId : metadata.name;

        std::string tempDir = getTempExportDir();
        if (!fs::exists(tempDir))
            fs::create_directories(tempDir);

        std::string outputFilename = workspaceName + ".tar.gz";
        std::string outputPath     = tempDir + outputFilename;

        auto result =
                services::WorkspaceService::exportWorkspace(workspaceId, getBaseDir(), outputPath);

        if (!result.success) {
            return makeError("Failed to export workspace", result.errorMessage);
        }

        // Return the file path
        Json::Value data;
        data["path"]     = outputPath;
        data["filename"] = outputFilename;

        std::cout << "Exported: " << workspaceName << " (ID: " << workspaceId << ")" << std::endl;

        return makeSuccess(data, "Workspace exported successfully");

    } catch (const std::exception &e) {
        std::cerr << "Export exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Workspace::listWorkspaces(const std::string &deviceId)
{
    auto result = services::WorkspaceService::listWorkspaces(getBaseDir(), deviceId);

    if (!result.success) {
        return makeError("Failed to list workspaces", result.errorMessage);
    }

    if (deviceId.empty()) {
        std::cout << "Listed " << result.data["count"].asInt() << " workspaces from all devices"
                  << std::endl;
    } else {
        std::cout << "Listed " << result.data["count"].asInt()
                  << " workspaces from device: " << deviceId << std::endl;
    }

    return makeSuccess(result.data);
}

helpers::CoreResult core::Workspace::createWorkspace(const std::string &name,
                                                     const std::string &target,
                                                     const std::string &description)
{
    if (name.empty()) {
        return makeError("Name is required");
    }

    services::WorkspaceMetadata metadata;
    metadata.id          = generateUuid();
    metadata.name        = name;
    metadata.target      = target;
    metadata.description = description;

    try {
        auto result = services::WorkspaceService::createWorkspace(getBaseDir(), metadata);

        if (!result.success) {
            return makeError("Failed to create workspace", result.errorMessage);
        }

        std::cout << "Created workspace: " << metadata.name << " (ID: " << metadata.id << ")"
                  << std::endl;

        return makeSuccess(result.data, "Workspace created successfully");

    } catch (const std::exception &e) {
        std::cerr << "Create exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Workspace::getWorkspaceInfo(const std::string &workspaceId)
{
    try {
        auto result = services::WorkspaceService::readWorkspace(getBaseDir(), workspaceId);

        if (!result.success) {
            return makeError("Workspace not found", result.errorMessage);
        }

        std::cout << "Retrieved workspace: " << workspaceId << std::endl;

        return makeSuccess(result.data);

    } catch (const std::exception &e) {
        std::cerr << "Get exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Workspace::updateWorkspace(const std::string &workspaceId,
                                                     const std::string &name,
                                                     const std::string &target,
                                                     const std::string &description)
{
    try {
        // Load existing metadata first
        auto existingResult = services::WorkspaceService::readWorkspace(getBaseDir(), workspaceId);
        if (!existingResult.success) {
            return makeError("Workspace not found", workspaceId);
        }

        // Prepare updated metadata, keeping existing values if not provided
        services::WorkspaceMetadata metadata;
        metadata.name = name.empty() ? existingResult.data["metadata"]["name"].asString() : name;
        metadata.target =
                target.empty() ? existingResult.data["metadata"]["target"].asString() : target;
        metadata.description = description.empty()
                                       ? existingResult.data["metadata"]["description"].asString()
                                       : description;

        auto result =
                services::WorkspaceService::updateWorkspace(getBaseDir(), workspaceId, metadata);

        if (!result.success) {
            return makeError("Workspace not found", result.errorMessage);
        }

        std::cout << "Updated workspace: " << workspaceId << std::endl;

        return makeSuccess(result.data, "Workspace updated successfully");

    } catch (const std::exception &e) {
        std::cerr << "Update exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Workspace::deleteWorkspace(const std::string &workspaceId)
{
    try {
        auto result = services::WorkspaceService::deleteWorkspace(getBaseDir(), workspaceId);

        if (!result.success) {
            return makeError("Workspace not found", result.errorMessage);
        }

        std::cout << "Deleted workspace: " << workspaceId << std::endl;

        return makeSuccess(result.data, "Workspace deleted successfully");

    } catch (const std::exception &e) {
        std::cerr << "Delete exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}
