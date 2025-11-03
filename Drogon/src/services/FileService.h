#pragma once

#include "ServiceResult.h"

#include <json/json.h>
#include <string>

namespace services
{

    class FileService
    {
      public:
        // File CRUD operations
        static ServiceResult createFile(const std::string &filePath, const std::string &content);
        static ServiceResult readFile(const std::string &filePath);
        static ServiceResult updateFile(const std::string &filePath, const std::string &content);
        static ServiceResult deleteFile(const std::string &filePath);
        static ServiceResult moveFile(const std::string &oldPath, const std::string &newPath);

        // File utility operations
        static Json::Value getFileInfo(const std::string &filePath);

      private:
    };

}  // namespace services
