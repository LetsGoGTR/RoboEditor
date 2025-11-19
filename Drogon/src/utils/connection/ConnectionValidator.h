#pragma once

#include <string>

#include "../../services/DeviceService.h"
#include "../../services/ServiceResult.h"
#include "../device/DeviceMetadataHelper.h"
#include "../sftp/SFTPClient.h"
#include "../sftp/SFTPConfig.h"

namespace utils
{

    enum class ConnectionErrorType
    {
        NONE,
        DNS_RESOLUTION_FAILED,
        CONNECTION_REFUSED,
        CONNECTION_TIMEOUT,
        AUTHENTICATION_FAILED,
        SFTP_INIT_FAILED,
        NETWORK_ERROR,
        HTTP_ERROR,
        UNKNOWN
    };

    struct ValidationResult
    {
        bool                success;
        std::string         errorMessage;
        std::string         errorDetail;
        ConnectionErrorType errorType;

        static ValidationResult createSuccess();
        static ValidationResult createError(const std::string   &message,
                                            ConnectionErrorType  type,
                                            const std::string   &detail = "");
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
                                                       bool              sshOnly  = true,
                                                       int               timeout  = 15);

        // Validate API connection (simple GET request)
        static ValidationResult validateAPIConnection(const std::string &apiUrl, int timeout = 10);

        // Validate both SFTP and API for a device
        static DeviceValidationResult validateDevice(const services::DeviceMetadata &metadata);
        static DeviceValidationResult validateDeviceFromParams(const ConnectionParams &params);

        // Format SFTP error message with detailed analysis
        static std::string formatSFTPError(const SFTPClient &client);

        // Parse error type from SFTP error message
        static ConnectionErrorType parseSFTPErrorType(const std::string &errorMessage);

        // Get user-friendly error description
        static std::string getErrorDescription(ConnectionErrorType errorType);

      private:
        // Helper to analyze error message and determine type
        static ConnectionErrorType analyzeErrorMessage(const std::string &errorMessage);
    };

}  // namespace utils
