#include "FileService.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

bool services::FileService::validateFilePath(const std::string& filePath) {
  if (filePath.empty()) {
    return false;
  }

  // Check for path traversal attacks
  if (filePath.find("..") != std::string::npos) {
    return false;
  }

  return true;
}

bool services::FileService::fileExists(const std::string& filePath) {
  try {
    return fs::exists(filePath) && fs::is_regular_file(filePath);
  } catch (const std::exception& e) {
    LOG_ERROR << "Error checking file existence: " << e.what();
    return false;
  }
}

bool services::FileService::isDirectory(const std::string& filePath) {
  try {
    return fs::exists(filePath) && fs::is_directory(filePath);
  } catch (const std::exception& e) {
    LOG_ERROR << "Error checking if directory: " << e.what();
    return false;
  }
}

std::string services::FileService::getFileExtension(
    const std::string& filePath) {
  return fs::path(filePath).extension().string();
}

int64_t services::FileService::getFileSize(const std::string& filePath) {
  try {
    if (fileExists(filePath)) {
      return fs::file_size(filePath);
    }
    return -1;
  } catch (const std::exception& e) {
    LOG_ERROR << "Error getting file size: " << e.what();
    return -1;
  }
}

Json::Value services::FileService::getFileInfo(const std::string& filePath) {
  Json::Value info;

  if (!fileExists(filePath)) {
    return info;
  }

  try {
    fs::path path(filePath);

    info["path"] = filePath;
    info["name"] = path.filename().string();
    info["extension"] = path.extension().string();
    info["size"] = (Json::Int64)fs::file_size(filePath);

    auto ftime = fs::last_write_time(filePath);
    auto sctp =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() +
            std::chrono::system_clock::now());
    auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
    info["modified"] = (Json::Int64)time_t_val;

  } catch (const std::exception& e) {
    LOG_ERROR << "Error getting file info: " << e.what();
  }

  return info;
}

services::FileOperationResult services::FileService::readFile(
    const std::string& filePath) {
  services::FileOperationResult result;
  result.success = false;

  if (!validateFilePath(filePath)) {
    result.errorMessage = "Invalid file path";
    LOG_WARN << "Invalid file path: " << filePath;
    return result;
  }

  if (!fileExists(filePath)) {
    result.errorMessage = "File does not exist";
    LOG_WARN << "File does not exist: " << filePath;
    return result;
  }

  try {
    std::ifstream file(filePath, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();

    result.data["content"] = buffer.str();
    result.data["info"] = getFileInfo(filePath);
    result.success = true;

    LOG_INFO << "Successfully read file: " << filePath;

  } catch (const std::exception& e) {
    result.errorMessage = "Failed to read file: " + std::string(e.what());
    LOG_ERROR << result.errorMessage;
  }

  return result;
}

services::FileOperationResult services::FileService::createFile(
    const std::string& filePath, const std::string& content) {
  services::FileOperationResult result;
  result.success = false;

  if (!validateFilePath(filePath)) {
    result.errorMessage = "Invalid file path";
    LOG_WARN << "Invalid file path: " << filePath;
    return result;
  }

  if (fileExists(filePath)) {
    result.errorMessage = "File already exists";
    LOG_WARN << "File already exists: " << filePath;
    return result;
  }

  try {
    // Create parent directories if they don't exist
    fs::path path(filePath);
    if (path.has_parent_path()) {
      fs::create_directories(path.parent_path());
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
      result.errorMessage = "Failed to create file";
      LOG_ERROR << "Failed to create file: " << filePath;
      return result;
    }

    file << content;
    file.close();

    result.data["info"] = getFileInfo(filePath);
    result.success = true;

    LOG_INFO << "Successfully created file: " << filePath;

  } catch (const std::exception& e) {
    result.errorMessage = "Failed to create file: " + std::string(e.what());
    LOG_ERROR << result.errorMessage;
  }

  return result;
}

services::FileOperationResult services::FileService::updateFile(
    const std::string& filePath, const std::string& content) {
  services::FileOperationResult result;
  result.success = false;

  if (!validateFilePath(filePath)) {
    result.errorMessage = "Invalid file path";
    LOG_WARN << "Invalid file path: " << filePath;
    return result;
  }

  if (!fileExists(filePath)) {
    result.errorMessage = "File does not exist";
    LOG_WARN << "File does not exist: " << filePath;
    return result;
  }

  try {
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
      result.errorMessage = "Failed to open file for writing";
      LOG_ERROR << "Failed to open file: " << filePath;
      return result;
    }

    file << content;
    file.close();

    result.data["info"] = getFileInfo(filePath);
    result.success = true;

    LOG_INFO << "Successfully updated file: " << filePath;

  } catch (const std::exception& e) {
    result.errorMessage = "Failed to update file: " + std::string(e.what());
    LOG_ERROR << result.errorMessage;
  }

  return result;
}

services::FileOperationResult services::FileService::deleteFile(
    const std::string& filePath) {
  services::FileOperationResult result;
  result.success = false;

  if (!validateFilePath(filePath)) {
    result.errorMessage = "Invalid file path";
    LOG_WARN << "Invalid file path: " << filePath;
    return result;
  }

  if (!fileExists(filePath)) {
    result.errorMessage = "File does not exist";
    LOG_WARN << "File does not exist: " << filePath;
    return result;
  }

  try {
    fs::remove(filePath);
    result.success = true;

    LOG_INFO << "Successfully deleted file: " << filePath;

  } catch (const std::exception& e) {
    result.errorMessage = "Failed to delete file: " + std::string(e.what());
    LOG_ERROR << result.errorMessage;
  }

  return result;
}
