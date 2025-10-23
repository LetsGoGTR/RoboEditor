#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    namespace v1
    {
        class Workspace : public HttpController<Workspace>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Workspace::workspaceImport, "/import", Post);
            METHOD_ADD(Workspace::workspaceExport, "/export", Post);
            METHOD_ADD(Workspace::workspaceList, "/list", Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void workspaceImport(const HttpRequestPtr                          &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback);
            void workspaceExport(const HttpRequestPtr                          &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback);
            void workspaceList(const HttpRequestPtr                          &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
