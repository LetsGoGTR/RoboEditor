#include "DeviceService.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

const std::string services::DeviceService::metadataFilename_ = "metadata.json";

// Helper functions
services::DeviceOperationResult services::DeviceService::createError(
    const std::string& message) {
  DeviceOperationResult result;
  result.success = false;
  result.errorMessage = message;
  LOG_ERROR << message;
  return result;
}

Json::Value services::DeviceMetadata::toJson() const {
  Json::Value json;
  json["id"] = id;
  json["name"] = name;
  json["description"] = description;
  json["ip"] = ip;
  json["createdAt"] = createdAt;
  json["updatedAt"] = updatedAt;
  return json;
}

services::DeviceMetadata services::DeviceMetadata::fromJson(
    const Json::Value& json) {
  DeviceMetadata metadata;

  if (json.isMember("id")) metadata.id = json["id"].asString();
  if (json.isMember("name")) metadata.name = json["name"].asString();
  if (json.isMember("description"))
    metadata.description = json["description"].asString();
  if (json.isMember("ip")) metadata.ip = json["ip"].asString();
  if (json.isMember("createdAt"))
    metadata.createdAt = json["createdAt"].asString();
  if (json.isMember("updatedAt"))
    metadata.updatedAt = json["updatedAt"].asString();

  return metadata;
}

std::string services::DeviceService::getCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

  return oss.str();
}

bool services::DeviceService::saveMetadata(const std::string& devicePath,
                                           const DeviceMetadata& metadata) {
  try {
    std::string metadataPath = devicePath + "/" + metadataFilename_;
    std::ofstream file(metadataPath);

    if (!file.is_open()) {
      LOG_ERROR << "Failed to open metadata file for writing: " << metadataPath;
      return false;
    }

    Json::Value json = metadata.toJson();
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    std::string jsonStr = Json::writeString(writer, json);

    file << jsonStr;
    file.close();

    LOG_INFO << "Metadata saved: " << metadataPath;
    return true;

  } catch (const std::exception& e) {
    LOG_ERROR << "Failed to save metadata: " << e.what();
    return false;
  }
}

services::DeviceMetadata services::DeviceService::loadMetadata(
    const std::string& devicePath) {
  DeviceMetadata metadata;

  try {
    std::string metadataPath = devicePath + "/" + metadataFilename_;

    if (!fs::exists(metadataPath)) {
      LOG_WARN << "Metadata file not found: " << metadataPath;
      return metadata;
    }

    std::ifstream file(metadataPath);
    if (!file.is_open()) {
      LOG_ERROR << "Failed to open metadata file: " << metadataPath;
      return metadata;
    }

    Json::Value json;
    Json::CharReaderBuilder reader;
    std::string errors;

    if (!Json::parseFromStream(reader, file, &json, &errors)) {
      LOG_ERROR << "Failed to parse metadata JSON: " << errors;
      file.close();
      return metadata;
    }

    file.close();
    metadata = DeviceMetadata::fromJson(json);

    LOG_INFO << "Metadata loaded: " << metadataPath;

  } catch (const std::exception& e) {
    LOG_ERROR << "Failed to load metadata: " << e.what();
  }

  return metadata;
}

services::DeviceOperationResult services::DeviceService::createDevice(
    const std::string& baseDir, const DeviceMetadata& metadata) {
  // Validation
  if (metadata.id.empty()) {
    return createError("Device ID cannot be empty");
  }

  std::string devicePath = baseDir + metadata.id;

  // Check if device already exists
  if (fs::exists(devicePath)) {
    return createError("Device already exists: " + metadata.id);
  }

  try {
    // Create device directory
    fs::create_directories(devicePath);

    // Prepare metadata with timestamps
    DeviceMetadata newMetadata = metadata;
    std::string timestamp = getCurrentTimestamp();
    newMetadata.createdAt = timestamp;
    newMetadata.updatedAt = timestamp;

    // Save metadata
    if (!saveMetadata(devicePath, newMetadata)) {
      fs::remove_all(devicePath);
      return createError("Failed to save metadata");
    }

    DeviceOperationResult result;
    result.success = true;
    result.data = newMetadata.toJson();

    LOG_INFO << "Created device: " << metadata.name
             << " (ID: " << metadata.id << ")";
    return result;

  } catch (const std::exception& e) {
    if (fs::exists(devicePath)) fs::remove_all(devicePath);
    return createError("Create failed: " + std::string(e.what()));
  }
}

services::DeviceOperationResult services::DeviceService::getDeviceInfo(
    const std::string& baseDir, const std::string& deviceId) {
  std::string devicePath = baseDir + deviceId;

  // Validation
  if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
    return createError("Device not found: " + deviceId);
  }

  DeviceMetadata metadata = loadMetadata(devicePath);
  if (metadata.id.empty()) {
    return createError("Invalid device metadata: " + deviceId);
  }

  DeviceOperationResult result;
  result.success = true;
  result.data = metadata.toJson();

  LOG_INFO << "Retrieved device info: " << deviceId;
  return result;
}

services::DeviceOperationResult services::DeviceService::updateDevice(
    const std::string& baseDir, const std::string& deviceId,
    const DeviceMetadata& metadata) {
  std::string devicePath = baseDir + deviceId;

  // Validation
  if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
    return createError("Device not found: " + deviceId);
  }

  // Load existing metadata to preserve createdAt
  DeviceMetadata existingMetadata = loadMetadata(devicePath);
  if (existingMetadata.id.empty()) {
    return createError("Invalid device metadata: " + deviceId);
  }

  try {
    // Prepare updated metadata
    DeviceMetadata updatedMetadata = metadata;
    updatedMetadata.id = deviceId;  // Ensure ID doesn't change
    updatedMetadata.createdAt = existingMetadata.createdAt;
    updatedMetadata.updatedAt = getCurrentTimestamp();

    // Save metadata
    if (!saveMetadata(devicePath, updatedMetadata)) {
      return createError("Failed to save metadata");
    }

    DeviceOperationResult result;
    result.success = true;
    result.data = updatedMetadata.toJson();

    LOG_INFO << "Updated device: " << deviceId;
    return result;

  } catch (const std::exception& e) {
    return createError("Update failed: " + std::string(e.what()));
  }
}

services::DeviceOperationResult services::DeviceService::deleteDevice(
    const std::string& baseDir, const std::string& deviceId) {
  std::string devicePath = baseDir + deviceId;

  // Validation
  if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
    return createError("Device not found: " + deviceId);
  }

  try {
    // Load metadata before deletion for response
    DeviceMetadata metadata = loadMetadata(devicePath);

    // Delete device directory
    fs::remove_all(devicePath);

    DeviceOperationResult result;
    result.success = true;
    result.data = metadata.toJson();

    LOG_INFO << "Deleted device: " << deviceId;
    return result;

  } catch (const std::exception& e) {
    return createError("Delete failed: " + std::string(e.what()));
  }
}
