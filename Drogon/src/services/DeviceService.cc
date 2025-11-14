#include "DeviceService.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../utils/ConfigUtils.h"
#include "../utils/JsonFileUtils.h"
#include "../utils/PathValidator.h"
#include "../utils/TimeUtils.h"
#include "../utils/logging/Logger.h"

namespace fs = std::filesystem;

const std::string services::DeviceService::metadataFilename_ = ".device.json";

Json::Value services::DeviceMetadata::toJson() const
{
    Json::Value json;
    json["id"]          = id;
    json["name"]        = name;
    json["description"] = description;
    json["ip"]          = ip;
    json["createdAt"]   = createdAt;
    json["updatedAt"]   = updatedAt;
    return json;
}

services::DeviceMetadata services::DeviceMetadata::fromJson(const Json::Value &json)
{
    DeviceMetadata metadata;

    if (json.isMember("id"))
        metadata.id = json["id"].asString();
    if (json.isMember("name"))
        metadata.name = json["name"].asString();
    if (json.isMember("description"))
        metadata.description = json["description"].asString();
    if (json.isMember("ip"))
        metadata.ip = json["ip"].asString();
    if (json.isMember("createdAt"))
        metadata.createdAt = json["createdAt"].asString();
    if (json.isMember("updatedAt"))
        metadata.updatedAt = json["updatedAt"].asString();

    return metadata;
}

services::DeviceMetadata services::DeviceService::loadMetadata(const std::string &devicePath)
{
    std::string metadataPath = devicePath + "/" + metadataFilename_;
    return utils::loadJsonFromFile<DeviceMetadata>(metadataPath);
}

services::ServiceResult services::DeviceService::createDevice(const DeviceMetadata &metadata)
{
    // Validate device ID
    if (metadata.id.empty()) {
        return ServiceResult::createError("Device ID cannot be empty");
    }

    // Validate path for security
    if (!utils::validatePath(metadata.id)) {
        utils::logging::warn("Invalid device ID: " + metadata.id);
        return ServiceResult::createError("Invalid device ID: " + metadata.id);
    }

    std::string devicePath = utils::config::getBaseDir() + metadata.id;

    // Check if device already exists
    if (fs::exists(devicePath) && fs::is_directory(devicePath)) {
        utils::logging::warn("Device already exists: " + metadata.id);
        return ServiceResult::createError("Device already exists: " + metadata.id);
    }

    try {
        // Create device directory
        fs::create_directories(devicePath);

        // Prepare metadata with timestamps
        DeviceMetadata newMetadata = metadata;
        std::string    timestamp   = utils::getCurrentTimestamp();
        newMetadata.createdAt      = timestamp;
        newMetadata.updatedAt      = timestamp;

        // Save metadata
        std::string metadataPath = devicePath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, newMetadata)) {
            fs::remove_all(devicePath);
            utils::logging::error("Failed to save device metadata: " + metadata.id);
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = newMetadata.toJson();

        utils::logging::info("Created device: " + metadata.name + " (ID: " + metadata.id + ")");
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(devicePath))
            fs::remove_all(devicePath);
        return ServiceResult::createError("Create failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::readDevice(const std::string &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(deviceId)) {
        utils::logging::warn("Invalid device ID: " + deviceId);
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string devicePath = utils::config::getBaseDir() + deviceId;

    // Check if device exists
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
        utils::logging::warn("Device not found: " + deviceId);
        return ServiceResult::createError("Device not found: " + deviceId);
    }

    DeviceMetadata metadata = loadMetadata(devicePath);
    if (metadata.id.empty()) {
        return ServiceResult::createError("Invalid device metadata: " + deviceId);
    }

    ServiceResult result;
    result.success = true;
    result.data    = metadata.toJson();

    utils::logging::info("Retrieved device info: " + deviceId);
    return result;
}

services::ServiceResult services::DeviceService::updateDevice(const std::string    &deviceId,
                                                              const DeviceMetadata &metadata)
{
    // Validate path for security
    if (!utils::validatePath(deviceId)) {
        utils::logging::warn("Invalid device ID: " + deviceId);
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string devicePath = utils::config::getBaseDir() + deviceId;

    // Check if device exists
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
        utils::logging::warn("Device not found: " + deviceId);
        return ServiceResult::createError("Device not found: " + deviceId);
    }

    // Load existing metadata to preserve createdAt
    DeviceMetadata existingMetadata = loadMetadata(devicePath);
    if (existingMetadata.id.empty()) {
        return ServiceResult::createError("Invalid device metadata: " + deviceId);
    }

    try {
        // Prepare updated metadata
        DeviceMetadata updatedMetadata = metadata;
        updatedMetadata.id             = deviceId;  // Ensure ID doesn't change
        updatedMetadata.createdAt      = existingMetadata.createdAt;
        updatedMetadata.updatedAt      = utils::getCurrentTimestamp();

        // Save metadata
        std::string metadataPath = devicePath + "/" + metadataFilename_;
        if (!utils::saveJsonToFile(metadataPath, updatedMetadata)) {
            utils::logging::error("Failed to save device metadata: " + deviceId);
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = updatedMetadata.toJson();

        utils::logging::info("Updated device: " + deviceId);
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Update failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::deleteDevice(const std::string &deviceId)
{
    // Validate path for security
    if (!utils::validatePath(deviceId)) {
        utils::logging::warn("Invalid device ID: " + deviceId);
        return ServiceResult::createError("Invalid device ID: " + deviceId);
    }

    std::string devicePath = utils::config::getBaseDir() + deviceId;

    // Check if device exists
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
        utils::logging::warn("Device not found: " + deviceId);
        return ServiceResult::createError("Device not found: " + deviceId);
    }

    try {
        // Load metadata before deletion for response
        DeviceMetadata metadata = loadMetadata(devicePath);

        // Delete device directory
        fs::remove_all(devicePath);

        ServiceResult result;
        result.success = true;
        result.data    = metadata.toJson();

        utils::logging::info("Deleted device: " + deviceId);
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Delete failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::listDevices()
{
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
        Json::Value devices(Json::arrayValue);

        for (const auto &entry : fs::directory_iterator(baseDir)) {
            if (!entry.is_directory())
                continue;

            DeviceMetadata metadata = loadMetadata(entry.path().string());
            if (metadata.id.empty())
                continue;  // Skip invalid

            devices.append(metadata.toJson());
        }

        ServiceResult result;
        result.success         = true;
        result.data["devices"] = devices;
        result.data["count"]   = (int)devices.size();

        utils::logging::info("Listed " + std::to_string(devices.size()) + " devices");
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("List failed: " + std::string(e.what()));
    }
}
