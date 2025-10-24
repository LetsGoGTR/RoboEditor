#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Workspace : public drogon::HttpController<Workspace>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Workspace::workspaceImport, "/import", drogon::Post);
            METHOD_ADD(Workspace::workspaceExport, "/export", drogon::Post);
            METHOD_ADD(Workspace::workspaceList, "/list", drogon::Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void workspaceImport(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void workspaceExport(const drogon::HttpRequestPtr                          &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void workspaceList(const drogon::HttpRequestPtr                          &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
