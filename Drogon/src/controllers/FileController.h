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
            METHOD_ADD(File::fileRead, "", Get);
            METHOD_ADD(File::fileCreate, "", Post);
            METHOD_ADD(File::fileUpdate, "", Put);
            METHOD_ADD(File::fileDelete, "", Delete);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void fileRead(const HttpRequestPtr                          &req,
                          std::function<void(const HttpResponsePtr &)> &&callback);
            void fileCreate(const HttpRequestPtr                          &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
            void fileUpdate(const HttpRequestPtr                          &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
            void fileDelete(const HttpRequestPtr                          &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
