#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>

namespace utils
{
    class RobotHttpClient
    {
      public:
        static void checkRunning(const std::string                             &scheme,
                                 const std::string                             &host,
                                 int                                            port,
                                 std::function<void(bool, const std::string &)> callback);

        static void uploadWorkspace(const std::string                             &scheme,
                                    const std::string                             &host,
                                    int                                            port,
                                    const std::string                             &filePath,
                                    std::function<void(bool, const std::string &)> callback);

        static void
        downloadWorkspace(const std::string                                            &scheme,
                          const std::string                                            &host,
                          int                                                           port,
                          std::function<void(const std::string &, const std::string &)> callback);

        static void
        archiveWorkspace(const std::string                                            &scheme,
                         const std::string                                            &host,
                         int                                                           port,
                         std::function<void(const std::string &, const std::string &)> callback);

        static void extractArchive(const std::string                             &scheme,
                                   const std::string                             &host,
                                   int                                            port,
                                   const std::string                             &remoteArchivePath,
                                   std::function<void(bool, const std::string &)> callback);

      private:
        // Robot API endpoints
        static constexpr const char *RUNNING_ENDPOINT = "/api/robot/running";
        static constexpr const char *IMPORT_ENDPOINT  = "/api/robot/import";
        static constexpr const char *EXPORT_ENDPOINT  = "/api/robot/export";
        static constexpr const char *ARCHIVE_ENDPOINT = "/api/robot/archive";
        static constexpr const char *EXTRACT_ENDPOINT = "/api/robot/extract";
    };

}  // namespace utils
