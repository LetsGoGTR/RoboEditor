#pragma once

#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include <string>
#include <filesystem>
#include <cstdlib>

namespace utils
{
    namespace config
    {
        namespace fs = std::filesystem;

        inline fs::path getPrefix()
        {
            if (const char* env = std::getenv("ROBOEDITOR_PREFIX"))
            {
                return fs::path(env);
            }

            #ifdef _WIN32
                // prefix가 안 넘어온 경우 기본값: C:\  (=> C:\roboeditor\...)
                return fs::path("C:\\");
            #else
                // 리눅스 기본: /   (=> /roboeditor/...)
                return fs::path("/");
            #endif
        }

        inline fs::path getRootDir()
        {
            return getPrefix() / "roboeditor";
        }

        inline const Json::Value& storageConfig()
        {
            const auto& cfg = drogon::app().getCustomConfig();
            static Json::Value dummy(Json::objectValue);
            if (!cfg.isMember("storage"))
            {
                LOG_ERROR << "[config] custom_config.storage not found";
                return dummy;
            }
            return cfg["storage"];
        }

        inline fs::path buildPath(const char* key, const char* defaultRelative)
        {
            const auto& storage = storageConfig();
            fs::path root = getRootDir();  // prefix + /roboeditor

            fs::path relative;

            if (storage.isMember(key) && storage[key].isString())
            {
                relative = fs::path(storage[key].asString());
            }
            else
            {
                relative = fs::path(defaultRelative);
            }

            return root / relative;  // prefix/roboeditor/<relative>
        }
        // Storage paths
        inline std::string getBaseDir()
        {
            return buildPath("base_dir", "storage").string();
        }

        inline std::string getTempUploadDir()
        {
            return buildPath("temp_upload_dir", "temp/upload").string();
        }

        inline std::string getTempExportDir()
        {
            return buildPath("temp_export_dir", "temp/export").string();
        }

        inline std::string getTempApplyDir()
        {
            return buildPath("temp_apply_dir", "temp/apply").string();
        }

        inline std::string getTempBackupDir()
        {
            return buildPath("temp_backup_dir", "temp/backup").string();
        }

    }  // namespace config

}  // namespace utils
