#include "WorkspaceService.h"

#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../utils/ConfigUtils.h"
#include "../utils/JsonFileUtils.h"
#include "../utils/PathValidator.h"
#include "../utils/TimeUtils.h"
#include "DeviceService.h"
#include "FolderService.h"
#include <drogon/drogon.h>

namespace fs = std::filesystem;

const std::vector<std::string> services::WorkspaceService::supportedFormats_ = {
        ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz"};

const std::string services::WorkspaceService::metadataFilename_ = ".workspace.json";

// Helper functions
bool services::WorkspaceService::isSupportedArchive(const std::string &filename)
{
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto &format : supportedFormats_) {
        if (lower.size() >= format.size() &&
            lower.compare(lower.size() - format.size(), format.size(), format) == 0) {
            return true;
        }
    }
    return false;
}

Json::Value services::WorkspaceMetadata::toJson() const
{
    Json::Value json;
    json["uuid"]        = uuid;
    json["target"]      = target;
    json["name"]        = name;
    json["description"] = description;
    json["createdAt"]   = createdAt;
    json["updatedAt"]   = updatedAt;
    return json;
}

services::WorkspaceMetadata services::WorkspaceMetadata::fromJson(const Json::Value &json)
{
    WorkspaceMetadata metadata;

    if (json.isMember("uuid"))
        metadata.uuid = json["uuid"].asString();
    if (json.isMember("target"))
        metadata.target = json["target"].asString();
    if (json.isMember("name"))
        metadata.name = json["name"].asString();
    if (json.isMember("description"))
        metadata.description = json["description"].asString();
    if (json.isMember("createdAt"))
        metadata.createdAt = json["createdAt"].asString();
    if (json.isMember("updatedAt"))
        metadata.updatedAt = json["updatedAt"].asString();

    return metadata;
}

services::WorkspaceMetadata
services::WorkspaceService::loadMetadata(const std::string &workspacePath)
{
    std::string metadataPath = workspacePath + "/" + metadataFilename_;
    return utils::loadJsonFromFile<WorkspaceMetadata>(metadataPath);
}

services::ServiceResult
services::WorkspaceService::importWorkspace(const std::string       &archivePath,
                                            const WorkspaceMetadata &metadata,
                                            const std::string       &deviceId)
{
    // Validate archive file
    if (!fs::exists(archivePath)) {
        LOG_WARN << "Archive file does not exist: " << archivePath;
        return ServiceResult::createError("Archive file does not exist: " + archivePath);
    }
    if (!isSupportedArchive(archivePath)) {
        LOG_WARN << "Unsupported archive format: " << archivePath;
        return ServiceResult::createError("Unsupported archive format: " + archivePath);
    }

    // Validate path for security
    if (!utils::validatePath(metadata.uuid)) {
        LOG_WARN << "Invalid workspace ID: " << metadata.uuid;
        return ServiceResult::createError("Invalid workspace ID: " + metadata.uuid);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string baseDir = utils::config::getBaseDir() + (deviceId.empty() ? "" : deviceId + "/");
    std::string workspacePath = baseDir + metadata.uuid;

    // Check if workspace already exists
    if (fs::exists(workspacePath) && fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace already exists: " << metadata.uuid;
        return ServiceResult::createError("Workspace already exists: " + metadata.uuid);
    }

    try {
        fs::create_directories(workspacePath);

        struct archive *a   = archive_read_new();
        struct archive *ext = archive_write_disk_new();

        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);
        archive_write_disk_set_standard_lookup(ext);

        if (archive_read_open_filename(a, archivePath.c_str(), 10240)) {
            std::string error = "Failed to open archive: " + std::string(archive_error_string(a));
            archive_read_free(a);
            archive_write_free(ext);
            fs::remove_all(workspacePath);
            return ServiceResult::createError(error);
        }

        struct archive_entry *entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            std::string fullPath = workspacePath + "/" + archive_entry_pathname(entry);
            archive_entry_set_pathname(entry, fullPath.c_str());

            if (archive_write_header(ext, entry) == ARCHIVE_OK && archive_entry_size(entry) > 0) {
                const void *buff;
                size_t      size;
                int64_t     offset;
                while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                    archive_write_data_block(ext, buff, size, offset);
                }
            }
            archive_write_finish_entry(ext);
        }

        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);

        std::string metadataPath = workspacePath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, metadata)) {
            fs::remove_all(workspacePath);
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Imported workspace: " << metadata.name << " (ID: " << metadata.uuid << ")";
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(workspacePath))
            fs::remove_all(workspacePath);
        return ServiceResult::createError("Import failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::WorkspaceService::exportWorkspace(const std::string &workspaceId,
                                                                    const std::string &outputPath,
                                                                    const std::string &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(workspaceId)) {
        LOG_WARN << "Invalid workspace ID: " << workspaceId;
        return ServiceResult::createError("Invalid workspace ID: " + workspaceId);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string baseDir = utils::config::getBaseDir() + (deviceId.empty() ? "" : deviceId + "/");
    std::string workspacePath = baseDir + workspaceId;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace not found: " << workspaceId;
        return ServiceResult::createError("Workspace not found: " + workspaceId);
    }

    WorkspaceMetadata metadata = loadMetadata(workspacePath);
    if (metadata.uuid.empty())
        return ServiceResult::createError("Invalid workspace metadata: " + workspaceId);

    try {
        struct archive *a = archive_write_new();
        archive_write_set_format_pax_restricted(a);
        archive_write_add_filter_gzip(a);

        if (archive_write_open_filename(a, outputPath.c_str()) != ARCHIVE_OK) {
            std::string error = "Failed to create archive: " + std::string(archive_error_string(a));
            archive_write_free(a);
            return ServiceResult::createError(error);
        }

        for (const auto &dirEntry : fs::recursive_directory_iterator(workspacePath)) {
            if (!dirEntry.is_regular_file())
                continue;

            std::string relativePath = fs::relative(dirEntry.path(), workspacePath).string();
            if (relativePath == metadataFilename_)
                continue;  // Skip metadata

            struct archive_entry *entry = archive_entry_new();
            archive_entry_set_pathname(entry, relativePath.c_str());
            archive_entry_set_size(entry, fs::file_size(dirEntry.path()));
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            archive_write_header(a, entry);

            std::ifstream file(dirEntry.path(), std::ios::binary);
            char          buff[8192];
            while (file.read(buff, sizeof(buff)) || file.gcount() > 0) {
                archive_write_data(a, buff, file.gcount());
            }

            archive_entry_free(entry);
        }

        archive_write_close(a);
        archive_write_free(a);

        ServiceResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Exported workspace: " << metadata.name << " (ID: " << workspaceId << ")";
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Export failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::WorkspaceService::listWorkspaces(const std::string &deviceId)
{
    // Validate device ID if provided
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string baseDir = utils::config::getBaseDir();

    if (!fs::exists(baseDir)) {
        try {
            fs::create_directories(baseDir);
        } catch (const std::exception &e) {
            return ServiceResult::createError("Failed to create base directory: " +
                                              std::string(e.what()));
        }
    }

    try {
        Json::Value workspaces(Json::arrayValue);

        if (deviceId.empty()) {
            // List workspaces from all devices
            for (const auto &deviceEntry : fs::directory_iterator(baseDir)) {
                if (!deviceEntry.is_directory())
                    continue;

                // Check if this is a valid device (has .device.json)
                std::string deviceMetadataPath = deviceEntry.path().string() + "/.device.json";
                if (!fs::exists(deviceMetadataPath))
                    continue;

                // Iterate through workspaces in this device
                for (const auto &workspaceEntry : fs::directory_iterator(deviceEntry.path())) {
                    if (!workspaceEntry.is_directory())
                        continue;

                    WorkspaceMetadata metadata = loadMetadata(workspaceEntry.path().string());
                    if (metadata.uuid.empty())
                        continue;  // Skip invalid

                    workspaces.append(metadata.toJson());
                }
            }
        } else {
            // List workspaces from specific device
            std::string devicePath = baseDir + deviceId;

            if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
                return ServiceResult::createError("Device not found: " + deviceId);
            }

            for (const auto &workspaceEntry : fs::directory_iterator(devicePath)) {
                if (!workspaceEntry.is_directory())
                    continue;

                WorkspaceMetadata metadata = loadMetadata(workspaceEntry.path().string());
                if (metadata.uuid.empty())
                    continue;  // Skip invalid

                workspaces.append(metadata.toJson());
            }
        }
        ServiceResult result;
        result.success            = true;
        result.data["workspaces"] = workspaces;
        result.data["count"]      = (int)workspaces.size();

        if (deviceId.empty()) {
            LOG_INFO << "Listed " << workspaces.size() << " workspaces from all devices";
        } else {
            LOG_INFO << "Listed " << workspaces.size() << " workspaces from device: " << deviceId;
        }
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("List failed: " + std::string(e.what()));
    }
}

// Get recursive directory tree structure
Json::Value services::WorkspaceService::getDirectoryTree(const std::string &path)
{
    Json::Value tree;

    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return tree;
        }

        tree["name"] = fs::path(path).filename().string();
        tree["path"] = path;
        tree["type"] = "directory";

        Json::Value children(Json::arrayValue);

        for (const auto &entry : fs::directory_iterator(path)) {
            // Skip metadata file
            if (entry.path().filename() == metadataFilename_) {
                continue;
            }

            Json::Value child;
            child["name"] = entry.path().filename().string();
            child["path"] = entry.path().string();

            if (entry.is_regular_file()) {
                child["type"] = "file";
                child["size"] = (Json::Int64)entry.file_size();
            } else if (entry.is_directory()) {
                child["type"] = "directory";
                // Recursive call for subdirectories
                Json::Value subtree = getDirectoryTree(entry.path().string());
                if (subtree.isMember("children")) {
                    child["children"] = subtree["children"];
                }
            }

            children.append(child);
        }

        tree["children"] = children;

    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to build directory tree: " << e.what();
    }

    return tree;
}

// Create empty workspace with metadata
services::ServiceResult
services::WorkspaceService::createWorkspace(const WorkspaceMetadata &metadata,
                                            const std::string       &deviceId)
{
    // Validate workspace ID
    if (metadata.uuid.empty()) {
        return ServiceResult::createError("Workspace ID cannot be empty");
    }

    // Validate path for security
    if (!utils::validatePath(metadata.uuid)) {
        LOG_WARN << "Invalid workspace ID: " << metadata.uuid;
        return ServiceResult::createError("Invalid workspace ID: " + metadata.uuid);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string paramsPath = (deviceId.empty() ? "" : deviceId + "/") + metadata.uuid;
    std::string workspacePath = utils::config::getBaseDir() + paramsPath;

    // Check if workspace already exists
    if (fs::exists(workspacePath) && fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace already exists: " << metadata.uuid;
        return ServiceResult::createError("Workspace already exists: " + metadata.uuid);
    }

    try {
        // Create workspace directory using FolderService
        auto folderResult = services::FolderService::createFolder(paramsPath);
        if (!folderResult.success) {
            return ServiceResult::createError("Failed to create workspace directory: " +
                                              folderResult.errorMessage);
        }

        // Prepare metadata with timestamps
        WorkspaceMetadata newMetadata = metadata;
        std::string       timestamp   = utils::getCurrentTimestamp();
        newMetadata.createdAt         = timestamp;
        newMetadata.updatedAt         = timestamp;

        // Save metadata
        std::string metadataPath = workspacePath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, newMetadata)) {
            fs::remove_all(workspacePath);
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = newMetadata.toJson();

        LOG_INFO << "Created workspace: " << metadata.name << " (ID: " << metadata.uuid << ")";
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(workspacePath))
            fs::remove_all(workspacePath);
        return ServiceResult::createError("Create failed: " + std::string(e.what()));
    }
}

// Get workspace metadata and tree structure
services::ServiceResult services::WorkspaceService::readWorkspace(const std::string &workspaceId,
                                                                  const std::string &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(workspaceId)) {
        LOG_WARN << "Invalid workspace ID: " << workspaceId;
        return ServiceResult::createError("Invalid workspace ID: " + workspaceId);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string baseDir = utils::config::getBaseDir() + (deviceId.empty() ? "" : deviceId + "/");
    std::string workspacePath = baseDir + workspaceId;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace not found: " << workspaceId;
        return ServiceResult::createError("Workspace not found: " + workspaceId);
    }

    WorkspaceMetadata metadata = loadMetadata(workspacePath);
    if (metadata.uuid.empty()) {
        return ServiceResult::createError("Invalid workspace metadata: " + workspaceId);
    }

    ServiceResult result;
    result.success          = true;
    result.data["metadata"] = metadata.toJson();
    result.data["tree"]     = getDirectoryTree(workspacePath);

    LOG_INFO << "Retrieved workspace: " << workspaceId;
    return result;
}

// Update workspace metadata
services::ServiceResult
services::WorkspaceService::updateWorkspace(const std::string       &workspaceId,
                                            const WorkspaceMetadata &metadata,
                                            const std::string       &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(workspaceId)) {
        LOG_WARN << "Invalid workspace ID: " << workspaceId;
        return ServiceResult::createError("Invalid workspace ID: " + workspaceId);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string baseDir = utils::config::getBaseDir() + (deviceId.empty() ? "" : deviceId + "/");
    std::string workspacePath = baseDir + workspaceId;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace not found: " << workspaceId;
        return ServiceResult::createError("Workspace not found: " + workspaceId);
    }

    // Load existing metadata to preserve createdAt and uuid
    WorkspaceMetadata existingMetadata = loadMetadata(workspacePath);
    if (existingMetadata.uuid.empty()) {
        return ServiceResult::createError("Invalid workspace metadata: " + workspaceId);
    }

    try {
        // Prepare updated metadata
        WorkspaceMetadata updatedMetadata = metadata;
        updatedMetadata.uuid              = workspaceId;  // Ensure uuid doesn't change
        updatedMetadata.createdAt         = existingMetadata.createdAt;
        updatedMetadata.updatedAt         = utils::getCurrentTimestamp();

        // Save metadata
        std::string metadataPath = workspacePath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, updatedMetadata)) {
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = updatedMetadata.toJson();

        LOG_INFO << "Updated workspace: " << workspaceId;
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Update failed: " + std::string(e.what()));
    }
}

// Delete workspace directory
services::ServiceResult services::WorkspaceService::deleteWorkspace(const std::string &workspaceId,
                                                                    const std::string &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(workspaceId)) {
        LOG_WARN << "Invalid workspace ID: " << workspaceId;
        return ServiceResult::createError("Invalid workspace ID: " + workspaceId);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string paramsPath = (deviceId.empty() ? "" : deviceId + "/") + workspaceId;
    std::string workspacePath = utils::config::getBaseDir() + paramsPath;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        LOG_WARN << "Workspace not found: " << workspaceId;
        return ServiceResult::createError("Workspace not found: " + workspaceId);
    }

    try {
        // Load metadata before deletion for response
        WorkspaceMetadata metadata = loadMetadata(workspacePath);

        // Delete workspace directory using FolderService
        auto folderResult = services::FolderService::deleteFolder(paramsPath);
        if (!folderResult.success) {
            return ServiceResult::createError("Failed to delete workspace directory: " +
                                              folderResult.errorMessage);
        }

        ServiceResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Deleted workspace: " << workspaceId;
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Delete failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::WorkspaceService::moveWorkspace(const std::string &workspaceId,
                                                                  const std::string &newWorkspaceId,
                                                                  const std::string &deviceId)
{
    // Validate paths for security
    if (!utils::validatePath(workspaceId)) {
        LOG_WARN << "Invalid workspace ID: " << workspaceId;
        return ServiceResult::createError("Invalid workspace ID: " + workspaceId);
    }
    if (!utils::validatePath(newWorkspaceId)) {
        LOG_WARN << "Invalid new workspace ID: " << newWorkspaceId;
        return ServiceResult::createError("Invalid new workspace ID: " + newWorkspaceId);
    }
    if (!deviceId.empty() && !utils::validatePath(deviceId)) {
        LOG_WARN << "Invalid device ID: " << deviceId;
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    // Validate workspace IDs are different
    if (workspaceId == newWorkspaceId) {
        return ServiceResult::createError("Source and destination workspace IDs must be different");
    }

    std::string baseDir = utils::config::getBaseDir() + (deviceId.empty() ? "" : deviceId + "/");
    std::string oldPath = baseDir + workspaceId;
    std::string newPath = baseDir + newWorkspaceId;

    // Check if source workspace exists
    if (!fs::exists(oldPath) || !fs::is_directory(oldPath)) {
        LOG_WARN << "Source workspace not found: " << workspaceId;
        return ServiceResult::createError("Source workspace not found: " + workspaceId);
    }

    // Check if destination already exists
    if (fs::exists(newPath) && fs::is_directory(newPath)) {
        LOG_WARN << "Destination workspace already exists: " << newWorkspaceId;
        return ServiceResult::createError("Destination workspace already exists: " +
                                          newWorkspaceId);
    }

    try {
        // Load metadata from old location
        WorkspaceMetadata metadata = loadMetadata(oldPath);

        // Move the workspace directory
        fs::rename(oldPath, newPath);

        // Update metadata with new ID and timestamp
        metadata.uuid        = newWorkspaceId;
        metadata.updatedAt = utils::getCurrentTimestamp();

        // Save updated metadata to new location
        std::string metadataPath = newPath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, metadata)) {
            // Rollback: move directory back
            fs::rename(newPath, oldPath);
            return ServiceResult::createError("Failed to update workspace metadata");
        }

        // Return success with both paths and updated metadata
        ServiceResult result;
        result.success         = true;
        result.data["oldPath"] = oldPath;
        result.data["newPath"] = newPath;
        result.data["oldId"]   = workspaceId;
        result.data["newId"]   = newWorkspaceId;
        result.data["info"]    = metadata.toJson();

        LOG_INFO << "Successfully moved workspace from: " << workspaceId << " to: " << newWorkspaceId;
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Failed to move workspace: " + std::string(e.what()));
    }
}
