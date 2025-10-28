#include "DeviceController.h"

void create(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {}

void info(const drogon::HttpRequestPtr& req,
          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
          const std::string& target) {}

void update(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& target) {}

void remove(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& target) {}

void upload(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& target) {}

void download(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& target) {}

void restore(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             const std::string& target) {}

void backup(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& target) {}
