#pragma once

#include <drogon/drogon.h>
#include <string>

namespace services
{

    struct FileOperationResult
    {
        bool        success;
        std::string errorMessage;
        Json::Value data;
    };

    class FileService
    {
      public:
        // File CRUD operations
        static FileOperationResult readFile(const std::string &filePath);
        static FileOperationResult createFile(const std::string &filePath,
                                              const std::string &content);
        static FileOperationResult updateFile(const std::string &filePath,
                                              const std::string &content);
        static FileOperationResult deleteFile(const std::string &filePath);

        // File utility operations
        static bool        fileExists(const std::string &filePath);
        static bool        isDirectory(const std::string &filePath);
        static std::string getFileExtension(const std::string &filePath);
        static int64_t     getFileSize(const std::string &filePath);
        static Json::Value getFileInfo(const std::string &filePath);

      private:
        static bool validateFilePath(const std::string &filePath);
    };

}  // namespace services
