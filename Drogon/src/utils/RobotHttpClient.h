#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <string>

namespace utils
{
    /**
     * RobotHttpClient - Abstraction for HTTP communication with Robot devices
     *
     * Provides simplified interfaces for common Robot API operations:
     * - Checking robot running status
     * - Uploading workspace files
     * - Downloading workspace files
     */
    class RobotHttpClient
    {
      public:
        /**
         * Check if robot is currently running
         * @param ip Robot IP address (e.g., "192.168.1.100:80" or "localhost:80")
         * @param callback Called with (isRunning, errorMessage)
         *                 - isRunning: true if robot is running, false otherwise
         *                 - errorMessage: empty if successful, error description otherwise
         */
        static void checkRunning(const std::string                              &ip,
                                 std::function<void(bool, const std::string &)>  callback);

        /**
         * Upload workspace file to robot
         * @param ip Robot IP address
         * @param filePath Path to workspace tar.gz file
         * @param callback Called with (success, errorMessage)
         *                 - success: true if upload succeeded
         *                 - errorMessage: empty if successful, error description otherwise
         */
        static void uploadWorkspace(const std::string                              &ip,
                                    const std::string                              &filePath,
                                    std::function<void(bool, const std::string &)>  callback);

        /**
         * Download workspace from robot
         * @param ip Robot IP address
         * @param callback Called with (content, errorMessage)
         *                 - content: workspace file content if successful, empty otherwise
         *                 - errorMessage: empty if successful, error description otherwise
         */
        static void downloadWorkspace(const std::string                                      &ip,
                                      std::function<void(const std::string &, const std::string &)>
                                              callback);

      private:
        // Robot API endpoints
        static constexpr const char *RUNNING_ENDPOINT = "/api/v1/robot/running";
        static constexpr const char *IMPORT_ENDPOINT  = "/api/v1/robot/import";
        static constexpr const char *EXPORT_ENDPOINT  = "/api/v1/robot/export";
    };

}  // namespace utils
