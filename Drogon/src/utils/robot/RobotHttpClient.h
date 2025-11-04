#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>

namespace utils
{
    class RobotHttpClient
    {
      public:
        static void checkRunning(const std::string                             &ip,
                                 std::function<void(bool, const std::string &)> callback);

        static void uploadWorkspace(const std::string                             &ip,
                                    const std::string                             &filePath,
                                    std::function<void(bool, const std::string &)> callback);

        static void
        downloadWorkspace(const std::string                                            &ip,
                          std::function<void(const std::string &, const std::string &)> callback);

      private:
        // Robot API endpoints
        static constexpr const char *RUNNING_ENDPOINT = "/api/v1/robot/running";
        static constexpr const char *IMPORT_ENDPOINT  = "/api/v1/robot/import";
        static constexpr const char *EXPORT_ENDPOINT  = "/api/v1/robot/export";
    };

}  // namespace utils
