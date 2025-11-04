#include "Logger.h"

#include <iostream>

void utils::logging::error(const std::string &message)
{
    std::cout << message;
}

void utils::logging::warn(const std::string &message)
{
    std::cout << message;
}

void utils::logging::info(const std::string &message)
{
    std::cout << message;
}

void utils::logging::debug(const std::string &message)
{
    std::cout << message;
}
