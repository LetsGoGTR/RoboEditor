#pragma once

#include <json/json.h>
#include <string>
#include <vector>

#include "ServiceResult.h"

namespace services
{

    struct WorkspaceMetadata
    {
        std::string uuid;
        std::string target;
        std::string name;
        std::string description;
        std::string createdAt;
        std::string updatedAt;

        Json::Value              toJson() const;
        static WorkspaceMetadata fromJson(const Json::Value &json);
    };

    class WorkspaceService
    {
      public:
        // Workspace CRUD operations
        static ServiceResult createWorkspace(const WorkspaceMetadata &metadata,
                                             const std::string       &deviceId = "");
        static ServiceResult readWorkspace(const std::string &workspaceId,
                                           const std::string &deviceId = "");
        static ServiceResult updateWorkspace(const std::string       &workspaceId,
                                             const WorkspaceMetadata &metadata,
                                             const std::string       &deviceId = "");
        static ServiceResult deleteWorkspace(const std::string &workspaceId,
                                             const std::string &deviceId = "");

        static ServiceResult moveWorkspace(const std::string &workspaceId,
                                           const std::string &newWorkspaceId,
                                           const std::string &deviceId = "");

        // List all workspaces
        static ServiceResult listWorkspaces(const std::string &deviceId = "");

        // Extract archive to UUID-based directory and create metadata
        static ServiceResult importWorkspace(const std::string       &archivePath,
                                             const WorkspaceMetadata &metadata,
                                             const std::string       &deviceId = "");

        // Compress workspace directory to archive
        static ServiceResult exportWorkspace(const std::string &workspaceId,
                                             const std::string &outputPath,
                                             const std::string &deviceId = "");

        // Check if archive format is supported
        static bool isSupportedArchive(const std::string &filename);

        // Load workspace metadata from .workspace.json
        static WorkspaceMetadata loadMetadata(const std::string &workspacePath);

      private:
        static const std::vector<std::string> supportedFormats_;
        static const std::string              metadataFilename_;

        // Helper functions
        static Json::Value getDirectoryTree(const std::string &path);
    };

}  // namespace services
