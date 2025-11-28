#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Validation : public drogon::HttpController<Validation>
        {
          public:
            METHOD_LIST_BEGIN
            METHOD_ADD(Validation::device, "/device", drogon::Post);
            METHOD_LIST_END

            void device(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
