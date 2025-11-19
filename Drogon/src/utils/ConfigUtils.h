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

    }  // namespace config

}  // namespace utils
