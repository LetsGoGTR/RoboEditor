#pragma once

#include <string>

#include "../../services/DeviceService.h"
#include "../../services/ServiceResult.h"
#include "../sftp/SFTPConfig.h"

namespace utils
{

    struct ConnectionParams
    {
        std::string sftpHost;
        int         sftpPort;
        std::string sftpUser;
        std::string sftpPassword;
        std::string apiUrl;  // "scheme://host:port"

        bool hasSFTPInfo() const;
        bool hasAPIInfo() const;
    };

    class DeviceMetadataHelper
    {
      public:
        // Extract connection parameters from device metadata
        static services::ServiceResult extractConnectionParams(
                const services::DeviceMetadata &metadata, ConnectionParams &params);

        // Extract connection parameters from JSON (from DeviceService::readDevice result)
        static services::ServiceResult extractConnectionParamsFromJson(const Json::Value &deviceData,
                                                                       ConnectionParams  &params);

        // Create SFTPConfig from ConnectionParams
        static SFTPConfig createSFTPConfig(const ConnectionParams &params, int timeout = 60);

        // Validation helpers
        static bool isValidIP(const std::string &ip);
        static bool isValidPort(int port);
    };

}  // namespace utils
