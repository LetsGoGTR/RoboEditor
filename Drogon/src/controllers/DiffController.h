#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    namespace v1
    {
        class Diff : public HttpController<Diff>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Diff::diffFiles, "/files", Post);
            METHOD_ADD(Diff::diffWorkspaces, "/workspaces", Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void diffFiles(const HttpRequestPtr                          &req,
                           std::function<void(const HttpResponsePtr &)> &&callback);
            void diffWorkspaces(const HttpRequestPtr                          &req,
                                std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
