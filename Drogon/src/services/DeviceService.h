#pragma once

#include <drogon/drogon.h>
#include <string>

namespace services
{

    struct DeviceMetadata
    {
        std::string id;
        std::string name;
        std::string description;
        std::string ip;
        std::string createdAt;
        std::string updatedAt;

        Json::Value           toJson() const;
        static DeviceMetadata fromJson(const Json::Value &json);
    };

    struct DeviceOperationResult
    {
        bool        success;
        std::string errorMessage;
        Json::Value data;
    };

    class DeviceService
    {
      public:
        // // Device CRUD operations
        static DeviceOperationResult createDevice(const std::string    &baseDir,
                                                  const DeviceMetadata &metadata);
        static DeviceOperationResult readDevice(const std::string &baseDir,
                                                const std::string &deviceId);
        static DeviceOperationResult updateDevice(const std::string    &baseDir,
                                                  const std::string    &deviceId,
                                                  const DeviceMetadata &metadata);
        static DeviceOperationResult deleteDevice(const std::string &baseDir,
                                                  const std::string &deviceId);

      private:
        static const std::string metadataFilename_;

        // Helper functions
        static DeviceOperationResult createError(const std::string &message);
        static bool saveMetadata(const std::string &devicePath, const DeviceMetadata &metadata);
        static DeviceMetadata loadMetadata(const std::string &devicePath);
        static std::string    getCurrentTimestamp();
    };

}  // namespace services
