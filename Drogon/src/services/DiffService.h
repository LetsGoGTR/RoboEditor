#pragma once

#include "ServiceResult.h"

#include <json/json.h>
#include <string>
#include <vector>

namespace services
{

    class DiffService
    {
      public:
        // Main diff function that automatically detects file type and routes to appropriate handler
        static ServiceResult diff(const std::string &contentA,
                                  const std::string &contentB,
                                  const std::string &nameA,
                                  const std::string &nameB);

        static ServiceResult diffDirectories(const std::string &dirA, const std::string &dirB);

      private:
        static const std::vector<std::string> supportedFormats_;

        // Check if a file format is supported
        static bool isSupportedFormat(const std::string &fileName);

        // Detect file type from extension
        static std::string detectFileType(const std::string &fileName);

        // Type-specific diff handlers
        static ServiceResult diffYaml(const std::string &contentA,
                                      const std::string &contentB,
                                      const std::string &nameA,
                                      const std::string &nameB);
        static ServiceResult diffPython(const std::string &contentA,
                                        const std::string &contentB,
                                        const std::string &nameA,
                                        const std::string &nameB);
        static ServiceResult diffText(const std::string &contentA,
                                      const std::string &contentB,
                                      const std::string &nameA,
                                      const std::string &nameB);
    };

}  // namespace services
