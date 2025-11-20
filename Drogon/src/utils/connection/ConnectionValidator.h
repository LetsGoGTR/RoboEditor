#pragma once

#include <string>

#include "../../services/DeviceService.h"
#include "../../services/ServiceResult.h"
#include "../device/DeviceMetadataHelper.h"
#include "../sftp/SFTPClient.h"
#include "../sftp/SFTPConfig.h"

namespace utils
{

    struct ValidationResult
    {
        bool        success;
        std::string errorMessage;

        static ValidationResult createSuccess()
        {
            return {true, ""};
        }

        static ValidationResult createError(const std::string &message)
        {
            return {false, message};
        }
    };

    struct DeviceValidationResult
    {
        bool             sftpValid;
        bool             apiValid;
        ValidationResult sftpResult;
        ValidationResult apiResult;

        bool isFullyValid() const
        {
            return sftpValid && apiValid;
        }

        Json::Value toJson() const;
    };

    class ConnectionValidator
    {
      public:
        // Validate SFTP connection
        // sshOnly: true = only test SSH handshake (faster), false = full SFTP init
        static ValidationResult validateSFTPConnection(const SFTPConfig &config,
                                                       bool              sshOnly = true,
                                                       int               timeout = 15);

        // Validate API connection (simple GET request)
        static ValidationResult validateAPIConnection(const std::string &apiUrl, int timeout = 10);

        // Validate both SFTP and API for a device
        static DeviceValidationResult validateDevice(const services::DeviceMetadata &metadata);
        static DeviceValidationResult validateDeviceFromParams(const ConnectionParams &params);
    };

}  // namespace utils
