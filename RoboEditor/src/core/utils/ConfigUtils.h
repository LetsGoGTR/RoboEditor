#pragma once

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
            return "/tmp/qt-app/storage/";
        }

        inline std::string getTempUploadDir()
        {
            return "/tmp/qt-app/temp/upload/";
        }

        inline std::string getTempExportDir()
        {
            return "/tmp/qt-app/temp/export/";
        }

        inline std::string getTempApplyDir()
        {
            return "/tmp/qt-app/temp/apply/";
        }

        inline std::string getTempBackupDir()
        {
            return "/tmp/qt-app/temp/backup/";
        }

        // Auth config
        inline std::string getPasswordHash()
        {
            return "$2b$12$V/CMuppdu98zKBfJOSKO/ueOcpAWVAyzBHzg9TAyqJ7Lqc/Pdm9yW";
        }

    }  // namespace config

}  // namespace utils
