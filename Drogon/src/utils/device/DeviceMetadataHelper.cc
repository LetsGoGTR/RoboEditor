#include "DeviceMetadataHelper.h"

#include <regex>

namespace utils
{

    bool ConnectionParams::hasSFTPInfo() const
    {
        return !sftpHost.empty() && sftpPort > 0 && !sftpUser.empty() && !sftpPassword.empty();
    }

    bool ConnectionParams::hasAPIInfo() const
    {
        return !apiUrl.empty();
    }

    services::ServiceResult DeviceMetadataHelper::extractConnectionParams(
            const services::DeviceMetadata &metadata, ConnectionParams &params)
    {
        // Validate host
        if (metadata.host.empty()) {
            return services::ServiceResult::createError("Device host is empty");
        }

        if (!isValidIP(metadata.host)) {
            return services::ServiceResult::createError("Invalid host format: " + metadata.host);
        }

        // Validate ports
        if (!isValidPort(metadata.apiPort)) {
            return services::ServiceResult::createError(
                    "Invalid API port: " + std::to_string(metadata.apiPort));
        }

        if (!isValidPort(metadata.sftpPort)) {
            return services::ServiceResult::createError(
                    "Invalid SFTP port: " + std::to_string(metadata.sftpPort));
        }

        // Extract parameters
        params.sftpHost     = metadata.host;
        params.sftpPort     = metadata.sftpPort;
        params.sftpUser     = metadata.sftpUser;
        params.sftpPassword = metadata.sftpPassword;

        // Build apiUrl with scheme
        std::string scheme = metadata.scheme.empty() ? "http" : metadata.scheme;
        params.apiUrl = scheme + "://" + metadata.host + ":" + std::to_string(metadata.apiPort);

        return services::ServiceResult::createSuccess("");
    }

    services::ServiceResult DeviceMetadataHelper::extractConnectionParamsFromJson(
            const Json::Value &deviceData, ConnectionParams &params)
    {
        // Extract from JSON
        std::string host         = deviceData["host"].asString();
        std::string scheme       = deviceData.isMember("scheme") ? deviceData["scheme"].asString() : "http";
        int         apiPort      = deviceData["apiPort"].asInt();
        int         sftpPort     = deviceData.isMember("sftpPort") ? deviceData["sftpPort"].asInt() : 22;
        std::string sftpPassword = deviceData["sftpPassword"].asString();
        std::string sftpUser     = deviceData["sftpUser"].asString();

        // Validate host
        if (host.empty()) {
            return services::ServiceResult::createError("Device host is empty");
        }

        if (!isValidIP(host)) {
            return services::ServiceResult::createError("Invalid host format: " + host);
        }

        // Validate ports
        if (!isValidPort(apiPort)) {
            return services::ServiceResult::createError("Invalid API port: " +
                                                        std::to_string(apiPort));
        }

        if (!isValidPort(sftpPort)) {
            return services::ServiceResult::createError("Invalid SFTP port: " +
                                                        std::to_string(sftpPort));
        }

        // Set parameters
        params.sftpHost     = host;
        params.sftpPort     = sftpPort;
        params.sftpUser     = sftpUser;
        params.sftpPassword = sftpPassword;
        params.apiUrl       = scheme + "://" + host + ":" + std::to_string(apiPort);

        return services::ServiceResult::createSuccess("");
    }

    SFTPConfig DeviceMetadataHelper::createSFTPConfig(const ConnectionParams &params, int timeout)
    {
        return SFTPConfig(params.sftpHost, params.sftpPort, params.sftpUser, params.sftpPassword,
                          timeout);
    }

    bool DeviceMetadataHelper::isValidIP(const std::string &ip)
    {
        if (ip.empty()) {
            return false;
        }

        // Simple IPv4 validation
        std::regex ipv4Regex(
                "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

        // Also allow hostname format (letters, numbers, dots, hyphens)
        std::regex hostnameRegex("^[a-zA-Z0-9][a-zA-Z0-9.-]*[a-zA-Z0-9]$|^[a-zA-Z0-9]$");

        return std::regex_match(ip, ipv4Regex) || std::regex_match(ip, hostnameRegex);
    }

    bool DeviceMetadataHelper::isValidPort(int port)
    {
        return port > 0 && port <= 65535;
    }

}  // namespace utils
