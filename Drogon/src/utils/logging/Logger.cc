#include "Logger.h"

#include <drogon/drogon.h>

void utils::logging::error(const std::string &message)
{
    LOG_ERROR << message;
}

void utils::logging::warn(const std::string &message)
{
    LOG_WARN << message;
}

void utils::logging::info(const std::string &message)
{
    LOG_INFO << message;
}

void utils::logging::debug(const std::string &message)
{
    LOG_DEBUG << message;
}
