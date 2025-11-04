#pragma once

#include <string>

namespace utils
{
    namespace logging
    {
        // Log an error message
        void error(const std::string &message);

        // Log a warning message
        void warn(const std::string &message);

        // Log an info message
        void info(const std::string &message);

        // Log a debug message
        void debug(const std::string &message);

    }  // namespace logging
}  // namespace utils
