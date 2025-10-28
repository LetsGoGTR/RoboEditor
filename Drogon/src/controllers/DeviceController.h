#pragma once

#include <drogon/HttpController.h>

namespace api
{
    namespace v1
    {
        class Device : public drogon::HttpController<Device>
        {
          public:
            METHOD_LIST_BEGIN
            // use METHOD_ADD to add your custom processing function here;
            METHOD_ADD(Device::create, "", drogon::Post);
            METHOD_ADD(Device::info, "/{target}", drogon::Get);
            METHOD_ADD(Device::update, "/{target}", drogon::Put);
            METHOD_ADD(Device::remove, "/{target}", drogon::Delete);
            METHOD_ADD(Device::upload, "/{target}/upload", drogon::Post);
            METHOD_ADD(Device::download, "/{target}/download", drogon::Get);
            METHOD_ADD(Device::restore, "/{target}/restore", drogon::Post);
            METHOD_ADD(Device::backup, "/{target}/backup", drogon::Get);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void create(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void info(const drogon::HttpRequestPtr                          &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      const std::string                                     &target);
            void update(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &target);
            void remove(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &target);
            void upload(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &target);
            void download(const drogon::HttpRequestPtr                          &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                          const std::string                                     &target);
            void restore(const drogon::HttpRequestPtr                          &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                         const std::string                                     &target);
            void backup(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &target);
        };
    }  // namespace v1
}  // namespace api
