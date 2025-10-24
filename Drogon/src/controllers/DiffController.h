#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Diff : public drogon::HttpController<Diff>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Diff::diffFiles, "/files", drogon::Post);
            METHOD_ADD(Diff::diffWorkspaces, "/workspaces", drogon::Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void diffFiles(const drogon::HttpRequestPtr                          &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void diffWorkspaces(const drogon::HttpRequestPtr                          &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
