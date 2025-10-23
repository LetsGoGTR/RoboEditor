#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    namespace v1
    {
        class File : public HttpController<File>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(File::upload, "/upload", Post);
            METHOD_ADD(File::listWorkspace, "/list", Get);
            METHOD_ADD(File::openFile, "/open", Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void upload(const HttpRequestPtr                          &req,
                        std::function<void(const HttpResponsePtr &)> &&callback);
            void listWorkspace(const HttpRequestPtr                          &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);
            void openFile(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
