#pragma once

#include "ServiceResult.h"

#include <json/json.h>
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

    class DeviceService
    {
      public:
        // Device CRUD operations
        static ServiceResult createDevice(const std::string    &baseDir,
                                          const DeviceMetadata &metadata);
        static ServiceResult readDevice(const std::string &baseDir, const std::string &deviceId);
        static ServiceResult updateDevice(const std::string    &baseDir,
                                          const std::string    &deviceId,
                                          const DeviceMetadata &metadata);
        static ServiceResult deleteDevice(const std::string &baseDir, const std::string &deviceId);

        // List all devices
        static ServiceResult listDevices(const std::string &baseDir);

      private:
        static const std::string metadataFilename_;

        // Helper functions
        static DeviceMetadata loadMetadata(const std::string &devicePath);
    };

}  // namespace services
