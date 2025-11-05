#pragma once

#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include <string>

#include "logging/Logger.h"

namespace utils
{
    namespace config
    {
        // Storage paths
        inline std::string getBaseDir()
        {
            return drogon::app().getCustomConfig()["storage"]["base_dir"].asString();
        }

        inline std::string getTempUploadDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_upload_dir"].asString();
        }

        inline std::string getTempExportDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_export_dir"].asString();
        }

        inline std::string getTempApplyDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_apply_dir"].asString();
        }

        inline std::string getTempBackupDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_backup_dir"].asString();
        }

        // Auth config
        inline std::string getPasswordHash()
        {
            try {
                const Json::Value &customConfig = drogon::app().getCustomConfig();

                if (!customConfig.isMember("auth") ||
                    !customConfig["auth"].isMember("device_password_hash")) {
                    utils::logging::error(
                            "Missing auth.device_password_hash in config.json custom_config");
                    return "";
                }

                std::string hash = customConfig["auth"]["device_password_hash"].asString();

                if (hash.empty()) {
                    utils::logging::error("device_password_hash is empty in config.json");
                    return "";
                }

                return hash;

            } catch (const std::exception &e) {
                utils::logging::error("Failed to load password hash from config: " +
                                      std::string(e.what()));
                return "";
            }
        }

    }  // namespace config

}  // namespace utils
