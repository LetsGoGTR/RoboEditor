#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    namespace v1
    {
        class Yaml : public HttpController<Yaml>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Yaml::diff, "/diff", Post);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void diff(const HttpRequestPtr                          &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);
        };
    }  // namespace v1
}  // namespace api
