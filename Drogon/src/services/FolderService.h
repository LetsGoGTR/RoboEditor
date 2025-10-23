#pragma once

#include <drogon/drogon.h>
#include <string>

using namespace drogon;

namespace services
{

    struct FolderOperationResult
    {
        bool        success;
        std::string errorMessage;
        Json::Value data;
    };

    class FolderService
    {
      public:
        // Folder CRUD operations
        static FolderOperationResult createFolder(const std::string &folderPath);
        static FolderOperationResult updateFolder(const std::string &oldPath,
                                                  const std::string &newPath);
        static FolderOperationResult deleteFolder(const std::string &folderPath);
        static FolderOperationResult readFolder(const std::string &folderPath);

        // Folder utility operations
        static bool        folderExists(const std::string &folderPath);
        static Json::Value getFolderInfo(const std::string &folderPath);

      private:
        static bool validateFolderPath(const std::string &folderPath);
    };

}  // namespace services
