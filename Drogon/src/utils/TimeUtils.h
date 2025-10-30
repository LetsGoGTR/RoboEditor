#pragma once

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

namespace utils
{
    inline std::string getCurrentTimestamp()
    {
        auto now        = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
                  1000;

        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

        return oss.str();
    }

    inline std::time_t fileTimeToUnixTime(const std::filesystem::file_time_type &ftime)
    {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now());
        return std::chrono::system_clock::to_time_t(sctp);
    }

}  // namespace utils
