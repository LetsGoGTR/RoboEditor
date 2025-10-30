#pragma once

#include "ServiceResult.h"

#include <drogon/drogon.h>
#include <string>

namespace services
{

    class FolderService
    {
      public:
        // Folder CRUD operations
        static ServiceResult createFolder(const std::string &folderPath);
        static ServiceResult readFolder(const std::string &folderPath);
        static ServiceResult updateFolder(const std::string &oldPath, const std::string &newPath);
        static ServiceResult deleteFolder(const std::string &folderPath);

      private:
        static Json::Value getFolderInfo(const std::string &folderPath);
    };

}  // namespace services
