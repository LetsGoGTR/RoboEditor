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
            METHOD_ADD(Device::list, "", drogon::Get);
            METHOD_ADD(Device::create, "", drogon::Post);
            METHOD_ADD(Device::info, "/{deviceId}", drogon::Get);
            METHOD_ADD(Device::update, "/{deviceId}", drogon::Put);
            METHOD_ADD(Device::remove, "/{deviceId}", drogon::Delete);
            METHOD_ADD(Device::apply, "/{deviceId}/apply", drogon::Post);
            METHOD_ADD(Device::backup, "/{deviceId}/backup", drogon::Get);
            METHOD_LIST_END
            // your declaration of processing function maybe like this:
            void list(const drogon::HttpRequestPtr                          &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void create(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
            void info(const drogon::HttpRequestPtr                          &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      const std::string                                     &deviceId);
            void update(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);
            void remove(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);
            void apply(const drogon::HttpRequestPtr                          &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       const std::string                                     &deviceId);
            void backup(const drogon::HttpRequestPtr                          &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                        const std::string                                     &deviceId);

          private:
            // Helper function to check if robot is running
            void checkRobotStatus(
                    const std::string                                                      &ip,
                    std::function<void()>                                                   onNotRunning,
                    std::shared_ptr<std::function<void(const drogon::HttpResponsePtr &)>>   callbackPtr);
        };
    }  // namespace v1
}  // namespace api
