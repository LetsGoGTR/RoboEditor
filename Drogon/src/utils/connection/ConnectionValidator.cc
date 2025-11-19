#include "ConnectionValidator.h"

#include <drogon/HttpClient.h>
#include <future>

#include "../logging/Logger.h"

namespace utils
{

    ValidationResult ValidationResult::createSuccess()
    {
        return {true, "", "", ConnectionErrorType::NONE};
    }

    ValidationResult ValidationResult::createError(const std::string   &message,
                                                   ConnectionErrorType  type,
                                                   const std::string   &detail)
    {
        return {false, message, detail, type};
    }

    Json::Value DeviceValidationResult::toJson() const
    {
        Json::Value json;
        json["sftpValid"] = sftpValid;
        json["apiValid"]  = apiValid;

        if (!sftpValid) {
            json["sftpError"]     = sftpResult.errorMessage;
            json["sftpErrorType"] = static_cast<int>(sftpResult.errorType);
            if (!sftpResult.errorDetail.empty()) {
                json["sftpErrorDetail"] = sftpResult.errorDetail;
            }
        }

        if (!apiValid) {
            json["apiError"]     = apiResult.errorMessage;
            json["apiErrorType"] = static_cast<int>(apiResult.errorType);
            if (!apiResult.errorDetail.empty()) {
                json["apiErrorDetail"] = apiResult.errorDetail;
            }
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
            std::string         errorMsg  = client.getLastError();
            ConnectionErrorType errorType = parseSFTPErrorType(errorMsg);

            return ValidationResult::createError(formatSFTPError(client), errorType, errorMsg);
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
                            ConnectionErrorType errorType = ConnectionErrorType::NETWORK_ERROR;
                            std::string         detail;

                            switch (result) {
                                case drogon::ReqResult::BadServerAddress:
                                    errorType = ConnectionErrorType::DNS_RESOLUTION_FAILED;
                                    detail    = "DNS resolution failed";
                                    break;
                                case drogon::ReqResult::Timeout:
                                    errorType = ConnectionErrorType::CONNECTION_TIMEOUT;
                                    detail    = "Connection timed out";
                                    break;
                                case drogon::ReqResult::NetworkFailure:
                                    errorType = ConnectionErrorType::CONNECTION_REFUSED;
                                    detail    = "Network failure";
                                    break;
                                default:
                                    detail = "Unknown network error";
                                    break;
                            }

                            promise.set_value(ValidationResult::createError(
                                    "API connection failed: " + detail, errorType, detail));
                            return;
                        }

                        // Connection successful (we don't care about the response content)
                        promise.set_value(ValidationResult::createSuccess());
                    },
                    static_cast<double>(timeout));

            // Wait for result with timeout
            auto status = future.wait_for(std::chrono::seconds(timeout + 5));
            if (status == std::future_status::timeout) {
                return ValidationResult::createError("API connection validation timed out",
                                                     ConnectionErrorType::CONNECTION_TIMEOUT,
                                                     "Future wait timeout");
            }

            return future.get();

        } catch (const std::exception &e) {
            return ValidationResult::createError("API connection error: " + std::string(e.what()),
                                                 ConnectionErrorType::UNKNOWN, e.what());
        }
    }

    DeviceValidationResult ConnectionValidator::validateDevice(
            const services::DeviceMetadata &metadata)
    {
        ConnectionParams params;
        auto extractResult = DeviceMetadataHelper::extractConnectionParams(metadata, params);

        if (!extractResult.success) {
            DeviceValidationResult result;
            result.sftpValid = false;
            result.apiValid  = false;
            result.sftpResult =
                    ValidationResult::createError(extractResult.errorMessage,
                                                  ConnectionErrorType::UNKNOWN, "Invalid metadata");
            result.apiResult = result.sftpResult;
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

    std::string ConnectionValidator::formatSFTPError(const SFTPClient &client)
    {
        std::string errorMsg  = client.getLastError();
        auto        errorType = parseSFTPErrorType(errorMsg);

        std::string prefix = "SFTP connection failed";
        std::string detail = getErrorDescription(errorType);

        if (!detail.empty()) {
            return prefix + ": " + detail;
        }

        return prefix + ": " + errorMsg;
    }

    ConnectionErrorType ConnectionValidator::parseSFTPErrorType(const std::string &errorMessage)
    {
        return analyzeErrorMessage(errorMessage);
    }

    std::string ConnectionValidator::getErrorDescription(ConnectionErrorType errorType)
    {
        switch (errorType) {
            case ConnectionErrorType::NONE:
                return "";
            case ConnectionErrorType::DNS_RESOLUTION_FAILED:
                return "Host not found (DNS resolution failed)";
            case ConnectionErrorType::CONNECTION_REFUSED:
                return "Connection refused (check IP and port)";
            case ConnectionErrorType::CONNECTION_TIMEOUT:
                return "Connection timed out (host may be unreachable)";
            case ConnectionErrorType::AUTHENTICATION_FAILED:
                return "Authentication failed (check username and password)";
            case ConnectionErrorType::SFTP_INIT_FAILED:
                return "SFTP initialization failed";
            case ConnectionErrorType::NETWORK_ERROR:
                return "Network error";
            case ConnectionErrorType::HTTP_ERROR:
                return "HTTP error";
            case ConnectionErrorType::UNKNOWN:
            default:
                return "Unknown error";
        }
    }

    ConnectionErrorType ConnectionValidator::analyzeErrorMessage(const std::string &errorMessage)
    {
        std::string lowerMsg = errorMessage;
        std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);

        // DNS/hostname resolution
        if (lowerMsg.find("resolve") != std::string::npos ||
            lowerMsg.find("dns") != std::string::npos ||
            lowerMsg.find("host not found") != std::string::npos ||
            lowerMsg.find("no such host") != std::string::npos ||
            lowerMsg.find("getaddrinfo") != std::string::npos) {
            return ConnectionErrorType::DNS_RESOLUTION_FAILED;
        }

        // Connection refused
        if (lowerMsg.find("connection refused") != std::string::npos ||
            lowerMsg.find("refused") != std::string::npos ||
            lowerMsg.find("econnrefused") != std::string::npos) {
            return ConnectionErrorType::CONNECTION_REFUSED;
        }

        // Timeout
        if (lowerMsg.find("timeout") != std::string::npos ||
            lowerMsg.find("timed out") != std::string::npos ||
            lowerMsg.find("etimedout") != std::string::npos) {
            return ConnectionErrorType::CONNECTION_TIMEOUT;
        }

        // Authentication
        if (lowerMsg.find("auth") != std::string::npos ||
            lowerMsg.find("password") != std::string::npos ||
            lowerMsg.find("permission denied") != std::string::npos ||
            lowerMsg.find("access denied") != std::string::npos ||
            lowerMsg.find("publickey") != std::string::npos) {
            return ConnectionErrorType::AUTHENTICATION_FAILED;
        }

        // SFTP specific
        if (lowerMsg.find("sftp") != std::string::npos &&
            (lowerMsg.find("init") != std::string::npos ||
             lowerMsg.find("session") != std::string::npos)) {
            return ConnectionErrorType::SFTP_INIT_FAILED;
        }

        // Network errors
        if (lowerMsg.find("network") != std::string::npos ||
            lowerMsg.find("socket") != std::string::npos ||
            lowerMsg.find("connect") != std::string::npos) {
            return ConnectionErrorType::NETWORK_ERROR;
        }

        return ConnectionErrorType::UNKNOWN;
    }

}  // namespace utils
