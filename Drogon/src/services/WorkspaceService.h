#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>

using namespace drogon;

namespace services
{

    struct WorkspaceOperationResult
    {
        bool        success;
        std::string errorMessage;
        Json::Value data;
    };

    class WorkspaceService
    {
      public:
        // Extract archive to baseDir
        static WorkspaceOperationResult importWorkspace(const std::string &archivePath,
                                                        const std::string &workspaceName);

        // Compress workspace directory to archive
        static WorkspaceOperationResult exportWorkspace(const std::string &workspacePath,
                                                        const std::string &outputPath);

        // List all workspaces in baseDir
        static WorkspaceOperationResult listWorkspaces(const std::string &baseDir);

        // Check if archive format is supported
        static bool isSupportedArchive(const std::string &filename);

      private:
        static const std::vector<std::string> supportedFormats_;
    };

}  // namespace services
