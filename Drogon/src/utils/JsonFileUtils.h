#pragma once

#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <string>
#include <type_traits>

#include <drogon/drogon.h>

namespace utils
{
    template <typename T>
    bool
    saveJsonToFile(const std::string &filePath, const T &object, const std::string &indent = "  ")
    {
        try {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                LOG_ERROR << "Failed to open file for writing: " << filePath;
                return false;
            }

            Json::Value               json = object.toJson();
            Json::StreamWriterBuilder writer;
            writer["indentation"] = indent;
            std::string jsonStr   = Json::writeString(writer, json);

            file << jsonStr;
            file.close();

            LOG_INFO << "JSON saved: " << filePath;
            return true;

        } catch (const std::exception &e) {
            LOG_ERROR << "Failed to save JSON: " << e.what();
            return false;
        }
    }

    template <typename T>
    T loadJsonFromFile(const std::string &filePath, const T &defaultValue = T{})
    {
        try {
            namespace fs = std::filesystem;

            if (!fs::exists(filePath)) {
                LOG_WARN << "JSON file not found: " << filePath;
                return defaultValue;
            }

            std::ifstream file(filePath);
            if (!file.is_open()) {
                LOG_ERROR << "Failed to open JSON file: " << filePath;
                return defaultValue;
            }

            Json::Value             json;
            Json::CharReaderBuilder reader;
            std::string             errors;

            if (!Json::parseFromStream(reader, file, &json, &errors)) {
                LOG_ERROR << "Failed to parse JSON: " << errors;
                file.close();
                return defaultValue;
            }

            file.close();
            T result = T::fromJson(json);

            LOG_INFO << "JSON loaded: " << filePath;
            return result;

        } catch (const std::exception &e) {
            LOG_ERROR << "Failed to load JSON: " << e.what();
            return defaultValue;
        }
    }

}  // namespace utils
