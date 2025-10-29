#include "WorkspaceService.h"

#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "DeviceService.h"
#include "FolderService.h"

namespace fs = std::filesystem;

const std::vector<std::string> services::WorkspaceService::supportedFormats_ = {
        ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz"};

const std::string services::WorkspaceService::metadataFilename_ = ".workspace.json";

// Helper functions
services::WorkspaceOperationResult
services::WorkspaceService::createError(const std::string &message)
{
    WorkspaceOperationResult result;
    result.success      = false;
    result.errorMessage = message;
    LOG_ERROR << message;
    return result;
}

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
    json["id"]          = id;
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

    if (json.isMember("id"))
        metadata.id = json["id"].asString();
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

std::string services::WorkspaceService::getCurrentTimestamp()
{
    auto now        = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

    return oss.str();
}

bool services::WorkspaceService::saveMetadata(const std::string       &workspacePath,
                                              const WorkspaceMetadata &metadata)
{
    try {
        std::string   metadataPath = workspacePath + "/" + metadataFilename_;
        std::ofstream file(metadataPath);

        if (!file.is_open()) {
            LOG_ERROR << "Failed to open metadata file for writing: " << metadataPath;
            return false;
        }

        Json::Value               json = metadata.toJson();
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        std::string jsonStr   = Json::writeString(writer, json);

        file << jsonStr;
        file.close();

        LOG_INFO << "Metadata saved: " << metadataPath;
        return true;

    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to save metadata: " << e.what();
        return false;
    }
}

services::WorkspaceMetadata
services::WorkspaceService::loadMetadata(const std::string &workspacePath)
{
    WorkspaceMetadata metadata;

    try {
        std::string metadataPath = workspacePath + "/" + metadataFilename_;

        if (!fs::exists(metadataPath)) {
            LOG_WARN << "Metadata file not found: " << metadataPath;
            return metadata;
        }

        std::ifstream file(metadataPath);
        if (!file.is_open()) {
            LOG_ERROR << "Failed to open metadata file: " << metadataPath;
            return metadata;
        }

        Json::Value             json;
        Json::CharReaderBuilder reader;
        std::string             errors;

        if (!Json::parseFromStream(reader, file, &json, &errors)) {
            LOG_ERROR << "Failed to parse metadata JSON: " << errors;
            file.close();
            return metadata;
        }

        file.close();
        metadata = WorkspaceMetadata::fromJson(json);

        LOG_INFO << "Metadata loaded: " << metadataPath;

    } catch (const std::exception &e) {
        LOG_ERROR << "Failed to load metadata: " << e.what();
    }

    return metadata;
}

services::WorkspaceOperationResult
services::WorkspaceService::importWorkspace(const std::string       &archivePath,
                                            const std::string       &baseDir,
                                            const WorkspaceMetadata &metadata)
{
    // Validation
    if (!fs::exists(archivePath))
        return createError("Archive file does not exist: " + archivePath);
    if (!isSupportedArchive(archivePath))
        return createError("Unsupported archive format: " + archivePath);

    std::string workspacePath = baseDir + metadata.id;
    if (fs::exists(workspacePath))
        return createError("Workspace already exists: " + metadata.id);

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
            return createError(error);
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

        if (!saveMetadata(workspacePath, metadata)) {
            fs::remove_all(workspacePath);
            return createError("Failed to save metadata");
        }

        WorkspaceOperationResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Imported workspace: " << metadata.name << " (ID: " << metadata.id << ")";
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(workspacePath))
            fs::remove_all(workspacePath);
        return createError("Import failed: " + std::string(e.what()));
    }
}

services::WorkspaceOperationResult services::WorkspaceService::exportWorkspace(
        const std::string &workspaceId, const std::string &baseDir, const std::string &outputPath)
{
    std::string workspacePath = baseDir + workspaceId;

    // Validation
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath))
        return createError("Workspace not found: " + workspaceId);

    WorkspaceMetadata metadata = loadMetadata(workspacePath);
    if (metadata.id.empty())
        return createError("Invalid workspace metadata: " + workspaceId);

    try {
        struct archive *a = archive_write_new();
        archive_write_set_format_pax_restricted(a);
        archive_write_add_filter_gzip(a);

        if (archive_write_open_filename(a, outputPath.c_str()) != ARCHIVE_OK) {
            std::string error = "Failed to create archive: " + std::string(archive_error_string(a));
            archive_write_free(a);
            return createError(error);
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

        WorkspaceOperationResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Exported workspace: " << metadata.name << " (ID: " << workspaceId << ")";
        return result;

    } catch (const std::exception &e) {
        return createError("Export failed: " + std::string(e.what()));
    }
}

services::WorkspaceOperationResult
services::WorkspaceService::listWorkspaces(const std::string &baseDir, const std::string &deviceId)
{
    if (!fs::exists(baseDir)) {
        try {
            fs::create_directories(baseDir);
        } catch (const std::exception &e) {
            return createError("Failed to create base directory: " + std::string(e.what()));
        }
    }

    try {
        Json::Value workspaces(Json::arrayValue);

        if (deviceId.empty()) {
            // List workspaces from all devices
            for (const auto &deviceEntry : fs::directory_iterator(baseDir)) {
                if (!deviceEntry.is_directory())
                    continue;

                // Check if this is a valid device (has .metadata.json)
                std::string deviceMetadataPath = deviceEntry.path().string() + "/.metadata.json";
                if (!fs::exists(deviceMetadataPath))
                    continue;

                // Iterate through workspaces in this device
                for (const auto &workspaceEntry : fs::directory_iterator(deviceEntry.path())) {
                    if (!workspaceEntry.is_directory())
                        continue;

                    WorkspaceMetadata metadata = loadMetadata(workspaceEntry.path().string());
                    if (metadata.id.empty())
                        continue;  // Skip invalid

                    workspaces.append(metadata.toJson());
                }
            }
        } else {
            // List workspaces from specific device
            std::string devicePath = baseDir + deviceId;

            if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
                return createError("Device not found: " + deviceId);
            }

            for (const auto &workspaceEntry : fs::directory_iterator(devicePath)) {
                if (!workspaceEntry.is_directory())
                    continue;

                WorkspaceMetadata metadata = loadMetadata(workspaceEntry.path().string());
                if (metadata.id.empty())
                    continue;  // Skip invalid

                workspaces.append(metadata.toJson());
            }
        }

        WorkspaceOperationResult result;
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
        return createError("List failed: " + std::string(e.what()));
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
services::WorkspaceOperationResult
services::WorkspaceService::createWorkspace(const std::string       &baseDir,
                                            const WorkspaceMetadata &metadata)
{
    // Validation
    if (metadata.id.empty()) {
        return createError("Workspace ID cannot be empty");
    }

    std::string workspacePath = baseDir + metadata.id;

    // Check if workspace already exists
    if (fs::exists(workspacePath)) {
        return createError("Workspace already exists: " + metadata.id);
    }

    try {
        // Create workspace directory using FolderService
        auto folderResult = services::FolderService::createFolder(workspacePath);
        if (!folderResult.success) {
            return createError("Failed to create workspace directory: " +
                               folderResult.errorMessage);
        }

        // Prepare metadata with timestamps
        WorkspaceMetadata newMetadata = metadata;
        std::string       timestamp   = getCurrentTimestamp();
        newMetadata.createdAt         = timestamp;
        newMetadata.updatedAt         = timestamp;

        // Save metadata
        if (!saveMetadata(workspacePath, newMetadata)) {
            fs::remove_all(workspacePath);
            return createError("Failed to save metadata");
        }

        WorkspaceOperationResult result;
        result.success = true;
        result.data    = newMetadata.toJson();

        LOG_INFO << "Created workspace: " << metadata.name << " (ID: " << metadata.id << ")";
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(workspacePath))
            fs::remove_all(workspacePath);
        return createError("Create failed: " + std::string(e.what()));
    }
}

// Get workspace metadata and tree structure
services::WorkspaceOperationResult
services::WorkspaceService::readWorkspace(const std::string &baseDir,
                                          const std::string &workspaceId)
{
    std::string workspacePath = baseDir + workspaceId;

    // Validation
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        return createError("Workspace not found: " + workspaceId);
    }

    WorkspaceMetadata metadata = loadMetadata(workspacePath);
    if (metadata.id.empty()) {
        return createError("Invalid workspace metadata: " + workspaceId);
    }

    WorkspaceOperationResult result;
    result.success          = true;
    result.data["metadata"] = metadata.toJson();
    result.data["tree"]     = getDirectoryTree(workspacePath);

    LOG_INFO << "Retrieved workspace: " << workspaceId;
    return result;
}

// Update workspace metadata
services::WorkspaceOperationResult
services::WorkspaceService::updateWorkspaceMetadata(const std::string       &baseDir,
                                                    const std::string       &workspaceId,
                                                    const WorkspaceMetadata &metadata)
{
    std::string workspacePath = baseDir + workspaceId;

    // Validation
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        return createError("Workspace not found: " + workspaceId);
    }

    // Load existing metadata to preserve createdAt and id
    WorkspaceMetadata existingMetadata = loadMetadata(workspacePath);
    if (existingMetadata.id.empty()) {
        return createError("Invalid workspace metadata: " + workspaceId);
    }

    try {
        // Prepare updated metadata
        WorkspaceMetadata updatedMetadata = metadata;
        updatedMetadata.id                = workspaceId;  // Ensure ID doesn't change
        updatedMetadata.createdAt         = existingMetadata.createdAt;
        updatedMetadata.updatedAt         = getCurrentTimestamp();

        // Save metadata
        if (!saveMetadata(workspacePath, updatedMetadata)) {
            return createError("Failed to save metadata");
        }

        WorkspaceOperationResult result;
        result.success = true;
        result.data    = updatedMetadata.toJson();

        LOG_INFO << "Updated workspace: " << workspaceId;
        return result;

    } catch (const std::exception &e) {
        return createError("Update failed: " + std::string(e.what()));
    }
}

// Delete workspace directory
services::WorkspaceOperationResult
services::WorkspaceService::deleteWorkspace(const std::string &baseDir,
                                            const std::string &workspaceId)
{
    std::string workspacePath = baseDir + workspaceId;

    // Validation
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        return createError("Workspace not found: " + workspaceId);
    }

    try {
        // Load metadata before deletion for response
        WorkspaceMetadata metadata = loadMetadata(workspacePath);

        // Delete workspace directory using FolderService
        auto folderResult = services::FolderService::deleteFolder(workspacePath);
        if (!folderResult.success) {
            return createError("Failed to delete workspace directory: " +
                               folderResult.errorMessage);
        }

        WorkspaceOperationResult result;
        result.success = true;
        result.data    = metadata.toJson();

        LOG_INFO << "Deleted workspace: " << workspaceId;
        return result;

    } catch (const std::exception &e) {
        return createError("Delete failed: " + std::string(e.what()));
    }
}
