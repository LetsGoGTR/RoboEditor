#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Repo : public drogon::HttpController<Repo>
        {
          public:
            METHOD_LIST_BEGIN
            // Device CRUD
            METHOD_ADD(Repo::list, "", drogon::Get);
            METHOD_ADD(Repo::create, "", drogon::Post);
            METHOD_ADD(Repo::info, "/{deviceId}", drogon::Get);
            METHOD_ADD(Repo::update, "/{deviceId}", drogon::Put);
            METHOD_ADD(Repo::remove, "/{deviceId}", drogon::Delete);

            // Workspace CRUD
            METHOD_ADD(Repo::createWorkspace, "/{deviceId}", drogon::Post);
            METHOD_ADD(Repo::workspaceInfo, "/{deviceId}/{workspaceId}", drogon::Get);
            METHOD_ADD(Repo::workspaceUpdate, "/{deviceId}/{workspaceId}", drogon::Put);
            METHOD_ADD(Repo::workspaceRemove, "/{deviceId}/{workspaceId}", drogon::Delete);
            METHOD_LIST_END

            // Device methods
            void list(const drogon::HttpRequestPtr                          &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void create(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void info(const drogon::HttpRequestPtr                          &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      const std::string                                     &deviceId);
            void update(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);
            void remove(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);

            // Workspace methods
            void createWorkspace(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 const std::string                                     &deviceId);
            void workspaceInfo(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                               const std::string                                     &deviceId,
                               const std::string                                     &workspaceId);
            void workspaceUpdate(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 const std::string                                     &deviceId,
                                 const std::string &workspaceId);
            void workspaceRemove(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 const std::string                                     &deviceId,
                                 const std::string &workspaceId);
        };
    }  // namespace v1
}  // namespace api
