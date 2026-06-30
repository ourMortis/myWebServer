#ifndef TIME_UTIL_HPP
#define TIME_UTIL_HPP

#include <chrono>
#include <string>
#include <format>

constexpr const char *TIME_ZONE = "Asia/Shanghai"; // 时区配置

using time_point_s = std::chrono::local_seconds;
using time_point_ms = std::chrono::local_time<std::chrono::milliseconds>;
using time_ms = std::chrono::milliseconds;

class TimeUtil
{
public:
    static time_point_s now_sec()
    {
        static const std::chrono::time_zone *tz = std::chrono::get_tzdb().locate_zone(TIME_ZONE);
        return tz->to_local(std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    }

    static time_point_ms now_ms()
    {
        static const std::chrono::time_zone *tz = std::chrono::get_tzdb().locate_zone(TIME_ZONE);
        return tz->to_local(std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()));
    }

    static std::string now_sec_string()
    {
        return std::format("[{:%Y-%m-%d_%H-%M-%S}]", now_sec());
    }
    // 必须保证传入的格式形如 [2026-06-26_05-03-37]
    static time_point_s parse_time_string(std::string_view s)
    {
        s = s.substr(1, s.size() - 2);
        time_point_s local_time;
        std::istringstream iss{std::string(s)};
        iss >> std::chrono::parse("%Y-%m-%d_%H-%M-%S", local_time);
        return local_time;
    }
};

#endif // TIME_UTIL_HPP