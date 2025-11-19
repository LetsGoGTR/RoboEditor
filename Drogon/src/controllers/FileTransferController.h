#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class FT : public drogon::HttpController<FT>
        {
          public:
            METHOD_LIST_BEGIN
            METHOD_ADD(FT::backup, "/backup", drogon::Post);
            METHOD_ADD(FT::apply, "/apply", drogon::Post);
            METHOD_ADD(FT::changePassword, "/password", drogon::Post);
            METHOD_LIST_END

            void backup(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void apply(const drogon::HttpRequestPtr                          &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void changePassword(const drogon::HttpRequestPtr                          &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
