#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>

using namespace drogon;

namespace services
{

    struct DiffResult
    {
        bool        success;
        Json::Value data;
        std::string errorMessage;
    };

    class DiffService
    {
      public:
        // Main diff function that automatically detects file type and routes to appropriate handler
        static DiffResult diff(const std::string &contentA,
                               const std::string &contentB,
                               const std::string &nameA,
                               const std::string &nameB);

        // Check if a file format is supported
        static bool isSupportedFormat(const std::string &fileName);

      private:
        static const std::vector<std::string> supportedFormats_;

        // Detect file type from extension
        static std::string detectFileType(const std::string &fileName);

        // Type-specific diff handlers
        static DiffResult diffYaml(const std::string &contentA,
                                   const std::string &contentB,
                                   const std::string &nameA,
                                   const std::string &nameB);
        static DiffResult diffPython(const std::string &contentA,
                                     const std::string &contentB,
                                     const std::string &nameA,
                                     const std::string &nameB);
        static DiffResult diffText(const std::string &contentA,
                                   const std::string &contentB,
                                   const std::string &nameA,
                                   const std::string &nameB);
    };

}  // namespace services
