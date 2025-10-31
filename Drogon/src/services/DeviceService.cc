#include "DeviceService.h"

#include "../utils/JsonFileUtils.h"
#include "../utils/TimeUtils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

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

services::ServiceResult
services::DeviceService::createDevice(const std::string &baseDir, const DeviceMetadata &metadata)
{
    // Validation
    if (metadata.id.empty()) {
        return ServiceResult::createError("Device ID cannot be empty");
    }

    std::string devicePath = baseDir + metadata.id;

    // Check if device already exists
    if (fs::exists(devicePath)) {
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
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = newMetadata.toJson();

        LOG_INFO << "Created device: " << metadata.name << " (ID: " << metadata.id << ")";
        return result;

    } catch (const std::exception &e) {
        if (fs::exists(devicePath))
            fs::remove_all(devicePath);
        return ServiceResult::createError("Create failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::readDevice(const std::string &baseDir,
                                                                    const std::string &deviceId)
{
    std::string devicePath = baseDir + deviceId;

    // Validation
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
        return ServiceResult::createError("Device not found: " + deviceId);
    }

    DeviceMetadata metadata = loadMetadata(devicePath);
    if (metadata.id.empty()) {
        return ServiceResult::createError("Invalid device metadata: " + deviceId);
    }

    ServiceResult result;
    result.success = true;
    result.data    = metadata.toJson();

    LOG_INFO << "Retrieved device info: " << deviceId;
    return result;
}

services::ServiceResult services::DeviceService::updateDevice(
        const std::string &baseDir, const std::string &deviceId, const DeviceMetadata &metadata)
{
    std::string devicePath = baseDir + deviceId;

    // Validation
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
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
            return ServiceResult::createError("Failed to save metadata");
        }

        ServiceResult result;
        result.success = true;
        result.data    = updatedMetadata.toJson();

        LOG_INFO << "Updated device: " << deviceId;
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Update failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::deleteDevice(const std::string &baseDir,
                                                                      const std::string &deviceId)
{
    std::string devicePath = baseDir + deviceId;

    // Validation
    if (!fs::exists(devicePath) || !fs::is_directory(devicePath)) {
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

        LOG_INFO << "Deleted device: " << deviceId;
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("Delete failed: " + std::string(e.what()));
    }
}

services::ServiceResult services::DeviceService::listDevices(const std::string &baseDir)
{
    if (!fs::exists(baseDir)) {
        try {
            fs::create_directories(baseDir);
        } catch (const std::exception &e) {
            return ServiceResult::createError("Failed to create base directory: " + std::string(e.what()));
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

        LOG_INFO << "Listed " << devices.size() << " devices";
        return result;

    } catch (const std::exception &e) {
        return ServiceResult::createError("List failed: " + std::string(e.what()));
    }
}
