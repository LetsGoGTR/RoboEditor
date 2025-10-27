#pragma once

#include <drogon/drogon.h>

#include <string>
#include <vector>

namespace services {

struct WorkspaceMetadata {
  std::string id;
  std::string target;
  std::string name;
  std::string description;
  std::string createdAt;
  std::string updatedAt;

  Json::Value toJson() const;
  static WorkspaceMetadata fromJson(const Json::Value& json);
};

struct WorkspaceOperationResult {
  bool success;
  std::string errorMessage;
  Json::Value data;
};

class WorkspaceService {
 public:
  // Extract archive to UUID-based directory and create metadata
  static WorkspaceOperationResult importWorkspace(
      const std::string& archivePath, const std::string& baseDir,
      const WorkspaceMetadata& metadata);

  // Compress workspace directory to archive
  static WorkspaceOperationResult exportWorkspace(
      const std::string& workspaceId, const std::string& baseDir,
      const std::string& outputPath);

  // List all workspaces in baseDir (reads .workspace.json files)
  static WorkspaceOperationResult listWorkspaces(const std::string& baseDir);

  // Check if archive format is supported
  static bool isSupportedArchive(const std::string& filename);

  // Save workspace metadata to .workspace.json
  static bool saveMetadata(const std::string& workspacePath,
                           const WorkspaceMetadata& metadata);

  // Load workspace metadata from .workspace.json
  static WorkspaceMetadata loadMetadata(const std::string& workspacePath);

  // Generate current timestamp in ISO 8601 format
  static std::string getCurrentTimestamp();

 private:
  static const std::vector<std::string> supportedFormats_;
  static const std::string metadataFilename_;

  // Helper functions
  static WorkspaceOperationResult createError(const std::string& message);
};

}  // namespace services
