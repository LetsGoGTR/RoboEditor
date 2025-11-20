#include "ConnectionValidator.h"

#include <drogon/HttpClient.h>
#include <future>

#include "../logging/Logger.h"

namespace utils
{

    Json::Value DeviceValidationResult::toJson() const
    {
        Json::Value json;
        json["sftpValid"] = sftpValid;
        json["apiValid"]  = apiValid;

        if (!sftpValid) {
            json["sftpError"] = sftpResult.errorMessage;
        }

        if (!apiValid) {
            json["apiError"] = apiResult.errorMessage;
        }

        return json;
    }

    ValidationResult ConnectionValidator::validateSFTPConnection(const SFTPConfig &config,
                                                                  bool              sshOnly,
                                                                  int               timeout)
    {
        // Create config with custom timeout
        SFTPConfig timeoutConfig = config;
        timeoutConfig.timeout    = timeout;

        SFTPClient client(timeoutConfig);

        bool connected = false;
        if (sshOnly) {
            connected = client.connectSSHOnly();
        } else {
            connected = client.connect();
        }

        if (!connected) {
            return ValidationResult::createError(client.getLastError());
        }

        client.disconnect();
        return ValidationResult::createSuccess();
    }

    ValidationResult ConnectionValidator::validateAPIConnection(const std::string &apiUrl,
                                                                 int               timeout)
    {
        try {
            auto client = drogon::HttpClient::newHttpClient(apiUrl);

            // Create a simple GET request to check connectivity
            auto req = drogon::HttpRequest::newHttpRequest();
            req->setMethod(drogon::Get);
            req->setPath("/");  // Root path for basic connectivity check

            std::promise<ValidationResult> promise;
            auto                           future = promise.get_future();

            // Set timeout
            client->sendRequest(
                    req,
                    [&promise](drogon::ReqResult result, const drogon::HttpResponsePtr &response) {
                        if (result != drogon::ReqResult::Ok) {
                            promise.set_value(ValidationResult::createError("API connection failed"));
                            return;
                        }
                        promise.set_value(ValidationResult::createSuccess());
                    },
                    static_cast<double>(timeout));

            // Wait for result with timeout
            auto status = future.wait_for(std::chrono::seconds(timeout + 5));
            if (status == std::future_status::timeout) {
                return ValidationResult::createError("Connection validation timed out");
            }

            return future.get();

        } catch (const std::exception &e) {
            return ValidationResult::createError(e.what());
        }
    }

    DeviceValidationResult ConnectionValidator::validateDevice(
            const services::DeviceMetadata &metadata)
    {
        ConnectionParams params;
        auto extractResult = DeviceMetadataHelper::extractConnectionParams(metadata, params);

        if (!extractResult.success) {
            DeviceValidationResult result;
            result.sftpValid  = false;
            result.apiValid   = false;
            result.sftpResult = ValidationResult::createError(extractResult.errorMessage);
            result.apiResult  = result.sftpResult;
            return result;
        }

        return validateDeviceFromParams(params);
    }

    DeviceValidationResult ConnectionValidator::validateDeviceFromParams(const ConnectionParams &params)
    {
        DeviceValidationResult result;

        // Validate SFTP if info is available
        if (params.hasSFTPInfo()) {
            SFTPConfig sftpConfig = DeviceMetadataHelper::createSFTPConfig(params, 15);
            result.sftpResult     = validateSFTPConnection(sftpConfig, true, 15);
            result.sftpValid      = result.sftpResult.success;
        } else {
            result.sftpValid  = true;  // Skip if no SFTP info
            result.sftpResult = ValidationResult::createSuccess();
        }

        // Validate API if info is available
        if (params.hasAPIInfo()) {
            result.apiResult = validateAPIConnection(params.apiUrl, 10);
            result.apiValid  = result.apiResult.success;
        } else {
            result.apiValid  = true;  // Skip if no API info
            result.apiResult = ValidationResult::createSuccess();
        }

        return result;
    }

}  // namespace utils
