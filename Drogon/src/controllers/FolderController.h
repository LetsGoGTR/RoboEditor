#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    namespace v1
    {
        class Folder : public HttpController<Folder>
        {
          public:
            METHOD_LIST_BEGIN
            METHOD_ADD(Folder::folderCreate, "", Post);
            METHOD_ADD(Folder::folderRead, "", Get);
            METHOD_ADD(Folder::folderUpdate, "", Put);
            METHOD_ADD(Folder::folderDelete, "", Delete);
            METHOD_LIST_END

            void folderCreate(const HttpRequestPtr                          &req,
                              std::function<void(const HttpResponsePtr &)> &&callback);
            void folderRead(const HttpRequestPtr                          &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
            void folderUpdate(const HttpRequestPtr                          &req,
                              std::function<void(const HttpResponsePtr &)> &&callback);
            void folderDelete(const HttpRequestPtr                          &req,
                              std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
