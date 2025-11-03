#include "DeviceController.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "../services/AuthService.h"
#include "../services/DeviceService.h"
#include "../services/WorkspaceService.h"
#include "../utils/ConfigUtils.h"
#include "../utils/RobotHttpClient.h"
#include "../utils/TimeUtils.h"

namespace fs = std::filesystem;

using helpers::makeError;
using helpers::makeSuccess;
using utils::config::getBaseDir;
using utils::config::getTempApplyDir;
using utils::config::getTempBackupDir;

// 간단한 UUID 생성 함수
static std::string generateUuid()
{
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex << ms.count();
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }

    return ss.str();
}

helpers::CoreResult core::Device::listDevices()
{
    try {
        auto result = services::DeviceService::listDevices(getBaseDir());

        if (!result.success)
            return makeError("Failed to list devices", result.errorMessage);

        std::cout << "Listed " << result.data["count"].asInt() << " devices" << std::endl;

        return makeSuccess(result.data);

    } catch (const std::exception &e) {
        std::cerr << "List exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Device::createDevice(const std::string &id,
                                               const std::string &name,
                                               const std::string &description,
                                               const std::string &ip)
{
    if (id.empty() || name.empty())
        return makeError("Device ID and name cannot be empty");

    try {
        services::DeviceMetadata metadata;
        metadata.id          = id;
        metadata.name        = name;
        metadata.description = description;
        metadata.ip          = ip;

        auto result = services::DeviceService::createDevice(getBaseDir(), metadata);

        if (!result.success)
            return makeError("Failed to create device", result.errorMessage);

        std::cout << "Created device: " << metadata.name << " (ID: " << metadata.id << ")"
                  << std::endl;

        return makeSuccess(result.data, "Device created successfully");

    } catch (const std::exception &e) {
        std::cerr << "Create exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Device::getDeviceInfo(const std::string &deviceId)
{
    try {
        auto result = services::DeviceService::readDevice(getBaseDir(), deviceId);

        if (!result.success)
            return makeError("Device not found", result.errorMessage);

        std::cout << "Retrieved device info: " << deviceId << std::endl;

        return makeSuccess(result.data);

    } catch (const std::exception &e) {
        std::cerr << "Info exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Device::updateDevice(const std::string &deviceId,
                                               const std::string &name,
                                               const std::string &description,
                                               const std::string &ip)
{
    try {
        // Load existing metadata first
        auto existingResult = services::DeviceService::readDevice(getBaseDir(), deviceId);
        if (!existingResult.success) {
            return makeError("Device not found", deviceId);
        }

        // Prepare updated metadata, keeping existing values if not provided
        services::DeviceMetadata metadata;
        metadata.name = name.empty() ? existingResult.data["name"].asString() : name;
        metadata.description =
                description.empty() ? existingResult.data["description"].asString() : description;
        metadata.ip = ip.empty() ? existingResult.data["ip"].asString() : ip;

        auto result = services::DeviceService::updateDevice(getBaseDir(), deviceId, metadata);

        if (!result.success)
            return makeError("Device not found", result.errorMessage);

        std::cout << "Updated device: " << deviceId << std::endl;

        return makeSuccess(result.data, "Device updated successfully");

    } catch (const std::exception &e) {
        std::cerr << "Update exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

helpers::CoreResult core::Device::deleteDevice(const std::string &deviceId)
{
    try {
        auto result = services::DeviceService::deleteDevice(getBaseDir(), deviceId);

        if (!result.success)
            return makeError("Device not found", result.errorMessage);

        std::cout << "Deleted device: " << deviceId << std::endl;

        return makeSuccess(result.data, "Device deleted successfully");

    } catch (const std::exception &e) {
        std::cerr << "Delete exception: " << e.what() << std::endl;
        return makeError("Internal server error", e.what());
    }
}

void core::Device::applyToDevice(const std::string                       &deviceId,
                                 const std::string                       &password,
                                 const std::string                       &workspaceId,
                                 std::function<void(helpers::CoreResult)> callback)
{
    if (password.empty()) {
        callback(makeError("Password is required"));
        return;
    }

    if (!services::AuthService::verifyDevicePassword(password)) {
        callback(makeError("Invalid password"));
        return;
    }

    if (workspaceId.empty()) {
        callback(makeError("Workspace ID is required"));
        return;
    }

    // Check if device exists
    auto deviceResult = services::DeviceService::readDevice(getBaseDir(), deviceId);
    if (!deviceResult.success) {
        callback(makeError("Device not found", deviceId));
        return;
    }

    // Get IP from device metadata
    std::string ip = deviceResult.data["ip"].asString();
    if (ip.empty()) {
        ip = "localhost:80";
    }

    // Check if robot is running before proceeding
    checkRobotStatus(
            ip,
            [callback, ip, deviceId, workspaceId]() {
                try {
                    // Export workspace to temp file
                    std::string tempDir = utils::config::getTempApplyDir();
                    if (!fs::exists(tempDir))
                        fs::create_directories(tempDir);

                    std::string tempFile      = tempDir + generateUuid() + ".tar.gz";
                    std::string deviceBaseDir = utils::config::getBaseDir() + deviceId + "/";

                    // Export workspace
                    auto exportResult = services::WorkspaceService::exportWorkspace(
                            workspaceId, deviceBaseDir, tempFile);

                    if (!exportResult.success) {
                        fs::remove(tempFile);
                        callback(
                                makeError("Failed to export workspace", exportResult.errorMessage));
                        return;
                    }

                    // Upload workspace to robot
                    auto *httpClient = new utils::RobotHttpClient();
                    httpClient->uploadWorkspace(
                            ip,
                            tempFile,
                            [callback, tempFile, deviceId, workspaceId, httpClient](
                                    bool success, const std::string &error) {
                                // Clean up temp file
                                fs::remove(tempFile);

                                // Clean up httpClient
                                httpClient->deleteLater();

                                if (!success) {
                                    callback(makeError("Failed to upload workspace to robot",
                                                       error));
                                    return;
                                }

                                Json::Value data;
                                data["target"]      = deviceId;
                                data["workspaceId"] = workspaceId;

                                std::cout << "Apply completed: " << deviceId
                                          << " (Workspace: " << workspaceId << ")" << std::endl;

                                callback(makeSuccess(data, "Apply completed successfully"));
                            });

                } catch (const std::exception &e) {
                    std::cerr << "Apply exception: " << e.what() << std::endl;
                    callback(makeError("Internal server error", e.what()));
                }
            },
            callback);
}

void core::Device::backupFromDevice(const std::string                       &deviceId,
                                    std::function<void(helpers::CoreResult)> callback)
{
    // Check if device exists
    auto deviceResult = services::DeviceService::readDevice(getBaseDir(), deviceId);
    if (!deviceResult.success) {
        callback(makeError("Device not found", deviceId));
        return;
    }

    // Get IP from device metadata (default: localhost:80)
    std::string ip = deviceResult.data["ip"].asString();
    if (ip.empty()) {
        ip = "localhost:80";
    }

    // Check if robot is running before proceeding
    checkRobotStatus(
            ip,
            [callback, ip, deviceId]() {
                // Download workspace from robot
                auto *httpClient = new utils::RobotHttpClient();
                httpClient->downloadWorkspace(
                        ip,
                        [callback, deviceId, httpClient](const std::string &content,
                                                         const std::string &error) {
                            // Clean up httpClient at the end
                            httpClient->deleteLater();

                            if (!error.empty()) {
                                callback(makeError("Failed to download workspace from robot",
                                                   error));
                                return;
                            }

                            try {
                                // Save archive to temp file
                                std::string tempDir = utils::config::getTempBackupDir();
                                if (!fs::exists(tempDir))
                                    fs::create_directories(tempDir);

                                std::string   tempFile = tempDir + generateUuid() + ".tar.gz";
                                std::ofstream file(tempFile, std::ios::binary);
                                file << content;
                                file.close();

                                // Generate workspace ID and name
                                std::string workspaceId = generateUuid();
                                std::string timestamp   = utils::getCurrentTimestamp();
                                std::string timestampStr =
                                        timestamp.substr(0, 19);  // YYYY-MM-DDTHH:MM:SS
                                std::replace(timestampStr.begin(), timestampStr.end(), 'T', '_');
                                std::replace(timestampStr.begin(), timestampStr.end(), ':', '-');
                                std::string workspaceName = deviceId + "_" + timestampStr;

                                // Create workspace metadata
                                services::WorkspaceMetadata metadata;
                                metadata.id          = workspaceId;
                                metadata.target      = deviceId;
                                metadata.name        = workspaceName;
                                metadata.description = "backup from " + deviceId;
                                metadata.createdAt   = timestamp;
                                metadata.updatedAt   = timestamp;

                                // Import workspace into device folder
                                std::string deviceBaseDir =
                                        utils::config::getBaseDir() + deviceId + "/";
                                auto importResult = services::WorkspaceService::importWorkspace(
                                        tempFile, deviceBaseDir, metadata);

                                fs::remove(tempFile);

                                if (!importResult.success) {
                                    callback(makeError("Failed to import workspace",
                                                       importResult.errorMessage));
                                    return;
                                }

                                std::cout << "Backup completed: " << deviceId
                                          << " (Workspace: " << workspaceId << ")" << std::endl;

                                callback(makeSuccess(importResult.data,
                                                     "Backup completed successfully"));

                            } catch (const std::exception &e) {
                                std::cerr << "Backup exception: " << e.what() << std::endl;
                                callback(makeError("Internal server error", e.what()));
                            }
                        });
            },
            callback);
}

void core::Device::checkRobotStatus(const std::string                       &ip,
                                    std::function<void()>                    onNotRunning,
                                    std::function<void(helpers::CoreResult)> onError)
{
    auto *httpClient = new utils::RobotHttpClient();
    httpClient->checkRunning(
            ip, [onError, onNotRunning, httpClient](bool isRunning, const std::string &error) {
                // Clean up httpClient
                httpClient->deleteLater();

                if (!error.empty()) {
                    onError(makeError("Failed to check robot status", error));
                    return;
                }

                if (isRunning) {
                    onError(makeError("Robot is running",
                                      "Cannot perform operation while robot is running"));
                    return;
                }

                // Robot is not running, proceed with operation
                onNotRunning();
            });
}
