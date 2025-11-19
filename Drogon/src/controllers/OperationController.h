#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Operation : public drogon::HttpController<Operation>
        {
          public:
            METHOD_LIST_BEGIN
            // Device operations
            METHOD_ADD(Operation::apply, "/device/{deviceId}/apply", drogon::Post);
            METHOD_ADD(Operation::backup, "/device/{deviceId}/backup", drogon::Get);

            // Workspace operations
            METHOD_ADD(Operation::workspaceImport, "/workspace/import", drogon::Post);
            METHOD_ADD(Operation::workspaceExport, "/workspace/export", drogon::Post);
            METHOD_LIST_END

            // Device operation methods
            void apply(const drogon::HttpRequestPtr                          &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       const std::string                                     &deviceId);
            void backup(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);

            // Workspace operation methods
            void workspaceImport(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void workspaceExport(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

          private:
            // Helper function to check if robot is running
            void
            checkRobotStatus(const std::string    &ip,
                             std::function<void()> onNotRunning,
                             std::shared_ptr<std::function<void(const drogon::HttpResponsePtr &)>>
                                     callbackPtr);
        };
    }  // namespace v1
}  // namespace api
