#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class File : public drogon::HttpController<File>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(File::fileRead, "", drogon::Get);
            METHOD_ADD(File::fileCreate, "", drogon::Post);
            METHOD_ADD(File::fileUpdate, "", drogon::Put);
            METHOD_ADD(File::fileDelete, "", drogon::Delete);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void fileRead(const drogon::HttpRequestPtr                          &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void fileCreate(const drogon::HttpRequestPtr                          &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void fileUpdate(const drogon::HttpRequestPtr                          &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void fileDelete(const drogon::HttpRequestPtr                          &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
