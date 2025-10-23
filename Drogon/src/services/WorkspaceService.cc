#include "WorkspaceService.h"

#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace services;

const std::vector<std::string> WorkspaceService::supportedFormats_ = {
        ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz"};

bool WorkspaceService::isSupportedArchive(const std::string &filename)
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

WorkspaceOperationResult WorkspaceService::importWorkspace(const std::string &archivePath,
                                                           const std::string &workspaceName)
{
    WorkspaceOperationResult result;
    result.success = false;

    // Check if archive exists
    if (!fs::exists(archivePath)) {
        result.errorMessage = "Archive file does not exist";
        LOG_ERROR << result.errorMessage << ": " << archivePath;
        return result;
    }

    // Check if archive format is supported
    if (!isSupportedArchive(archivePath)) {
        result.errorMessage = "Unsupported archive format";
        LOG_ERROR << result.errorMessage << ": " << archivePath;
        return result;
    }

    try {
        // Create extraction directory
        if (!fs::exists(workspaceName)) {
            fs::create_directories(workspaceName);
        }

        struct archive       *a;
        struct archive       *ext;
        struct archive_entry *entry;
        int                   r;

        a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);

        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);
        archive_write_disk_set_standard_lookup(ext);

        if ((r = archive_read_open_filename(a, archivePath.c_str(), 10240))) {
            result.errorMessage = "Failed to open archive: " + std::string(archive_error_string(a));
            LOG_ERROR << result.errorMessage;
            archive_read_free(a);
            archive_write_free(ext);
            return result;
        }

        Json::Value extractedFiles(Json::arrayValue);

        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            std::string currentFile    = archive_entry_pathname(entry);
            std::string fullOutputPath = workspaceName + "/" + currentFile;

            archive_entry_set_pathname(entry, fullOutputPath.c_str());

            LOG_DEBUG << "Extracting: " << currentFile << " to " << fullOutputPath;

            r = archive_write_header(ext, entry);
            if (r != ARCHIVE_OK) {
                LOG_WARN << "Write header failed: " << archive_error_string(ext);
            } else {
                if (archive_entry_size(entry) > 0) {
                    const void *buff;
                    size_t      size;
                    int64_t     offset;

                    while (true) {
                        r = archive_read_data_block(a, &buff, &size, &offset);
                        if (r == ARCHIVE_EOF) {
                            break;
                        }
                        if (r != ARCHIVE_OK) {
                            LOG_ERROR << "Read data failed: " << archive_error_string(a);
                            break;
                        }
                        r = archive_write_data_block(ext, buff, size, offset);
                        if (r != ARCHIVE_OK) {
                            LOG_ERROR << "Write data failed: " << archive_error_string(ext);
                            break;
                        }
                    }
                }
            }

            archive_write_finish_entry(ext);
            extractedFiles.append(currentFile);
        }

        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);

        result.success                = true;
        result.data["workspaceName"]  = workspaceName;
        result.data["extractedFiles"] = extractedFiles;
        result.data["totalFiles"]     = (int)extractedFiles.size();

        LOG_INFO << "Successfully imported workspace: " << workspaceName << " with "
                 << extractedFiles.size() << " files";

    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Failed to import workspace: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}

WorkspaceOperationResult WorkspaceService::exportWorkspace(const std::string &workspacePath,
                                                           const std::string &outputPath)
{
    WorkspaceOperationResult result;
    result.success = false;

    // Check if workspace exists
    if (!fs::exists(workspacePath) || !fs::is_directory(workspacePath)) {
        result.errorMessage = "Workspace directory does not exist";
        LOG_ERROR << result.errorMessage << ": " << workspacePath;
        return result;
    }

    try {
        struct archive       *a;
        struct archive_entry *entry;

        a = archive_write_new();
        archive_write_set_format_pax_restricted(a);  // tar format
        archive_write_add_filter_gzip(a);

        if (archive_write_open_filename(a, outputPath.c_str()) != ARCHIVE_OK) {
            result.errorMessage =
                    "Failed to create archive: " + std::string(archive_error_string(a));
            LOG_ERROR << result.errorMessage;
            archive_write_free(a);
            return result;
        }

        Json::Value compressedFiles(Json::arrayValue);

        // Iterate through all files in workspace
        for (const auto &dirEntry : fs::recursive_directory_iterator(workspacePath)) {
            if (!dirEntry.is_regular_file()) {
                continue;
            }

            std::string filePath     = dirEntry.path().string();
            std::string relativePath = fs::relative(dirEntry.path(), workspacePath).string();

            LOG_DEBUG << "Adding to archive: " << relativePath;

            entry = archive_entry_new();
            archive_entry_set_pathname(entry, relativePath.c_str());
            archive_entry_set_size(entry, fs::file_size(filePath));
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);

            archive_write_header(a, entry);

            // Read and write file content
            std::ifstream file(filePath, std::ios::binary);
            char          buff[8192];
            while (file.read(buff, sizeof(buff)) || file.gcount() > 0) {
                archive_write_data(a, buff, file.gcount());
            }
            file.close();

            archive_entry_free(entry);
            compressedFiles.append(relativePath);
        }

        archive_write_close(a);
        archive_write_free(a);

        result.success                 = true;
        result.data["outputPath"]      = outputPath;
        result.data["compressedFiles"] = compressedFiles;
        result.data["totalFiles"]      = (int)compressedFiles.size();

        LOG_INFO << "Successfully exported workspace: " << workspacePath << " to " << outputPath
                 << " with " << compressedFiles.size() << " files";

    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Failed to export workspace: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}

WorkspaceOperationResult WorkspaceService::listWorkspaces(const std::string &baseDir)
{
    WorkspaceOperationResult result;
    result.success = false;

    // Check if baseDir exists
    if (!fs::exists(baseDir)) {
        try {
            fs::create_directories(baseDir);
        } catch (const std::exception &e) {
            result.errorMessage = "Failed to create base directory: " + std::string(e.what());
            LOG_ERROR << result.errorMessage;
            return result;
        }
    }

    try {
        Json::Value workspaces(Json::arrayValue);

        for (const auto &entry : fs::directory_iterator(baseDir)) {
            if (entry.is_directory()) {
                Json::Value workspace(Json::objectValue);
                workspace["name"] = entry.path().filename().string();
                workspace["path"] = entry.path().string();

                // Count files and subdirectories
                int fileCount = 0;
                int dirCount  = 0;

                for (const auto &subEntry : fs::recursive_directory_iterator(entry.path())) {
                    if (subEntry.is_regular_file()) {
                        fileCount++;
                    } else if (subEntry.is_directory()) {
                        dirCount++;
                    }
                }

                workspace["fileCount"]      = fileCount;
                workspace["directoryCount"] = dirCount;

                workspaces.append(workspace);
            }
        }

        result.success            = true;
        result.data["workspaces"] = workspaces;
        result.data["count"]      = (int)workspaces.size();

        LOG_INFO << "Listed " << workspaces.size() << " workspaces from " << baseDir;

    } catch (const std::exception &e) {
        result.success      = false;
        result.errorMessage = "Failed to list workspaces: " + std::string(e.what());
        LOG_ERROR << result.errorMessage;
    }

    return result;
}
