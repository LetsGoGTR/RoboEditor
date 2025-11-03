#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Folder : public drogon::HttpController<Folder>
        {
          public:
            METHOD_LIST_BEGIN
            METHOD_ADD(Folder::folderCreate, "", drogon::Post);
            METHOD_ADD(Folder::folderRead, "", drogon::Get);
            METHOD_ADD(Folder::folderUpdate, "", drogon::Put);
            METHOD_ADD(Folder::folderDelete, "", drogon::Delete);
            METHOD_LIST_END

            void folderCreate(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void folderRead(const drogon::HttpRequestPtr                          &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void folderUpdate(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void folderDelete(const drogon::HttpRequestPtr                          &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
