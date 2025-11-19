#pragma once

#include <json/json.h>
#include <string>

#include "ServiceResult.h"

namespace services
{

    struct DeviceMetadata
    {
        std::string serialNumber;
        std::string host;
        std::string scheme;
        int         apiPort;
        int         sftpPort;
        std::string sftpPassword;
        std::string sftpUser;
        std::string name;
        std::string description;
        std::string createdAt;
        std::string updatedAt;

        Json::Value           toJson() const;
        static DeviceMetadata fromJson(const Json::Value &json);
    };

    class DeviceService
    {
      public:
        // Device CRUD operations
        static ServiceResult createDevice(const DeviceMetadata &metadata);
        static ServiceResult readDevice(const std::string &deviceId);
        static ServiceResult updateDevice(const std::string    &deviceId,
                                          const DeviceMetadata &metadata);
        static ServiceResult deleteDevice(const std::string &deviceId);

        // List all devices
        static ServiceResult listDevices();

      private:
        static const std::string metadataFilename_;

        // Helper functions
        static DeviceMetadata loadMetadata(const std::string &devicePath);
    };

}  // namespace services
