#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <type_traits>
#include <format>
#include <chrono>
#include <assert.h>
#include <sstream>

class TestUtils
{
public:
    // 生成 count 个随机字符串
    // 每个长度在 [min_len, max_len] 之间
    static inline std::vector<std::string> get_str(size_t min_len, size_t max_len, size_t count)
    {
        static const char charset[] =
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<> char_dist(0, static_cast<int>(sizeof(charset)) - 2);

        std::vector<std::string> res;
        res.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            size_t len = len_dist(gen);
            std::string str;
            str.reserve(len);
            for (size_t j = 0; j < len; ++j)
            {
                str += charset[char_dist(gen)];
            }
            res.push_back(std::move(str));
        }
        return res;
    }

    // 生成 [min, max] 范围内的随机数
    template <typename T>
    static T random(T min, T max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        if constexpr (std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(gen);
        }
        else
        {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(gen);
        }
    }
    static inline std::string get_now_time_string_()
    {
        const auto now = std::chrono::system_clock::now();
        return std::format("[{:%Y-%m-%d_%H-%M-%S}]", now);
    }

    using time_point_s = std::chrono::local_seconds;
    time_point_s now_sec()
    {
        auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        return time_zone_->to_local(now);
    }

    // 必须保证传入的格式形如 [2026-06-26_05-03-37]
    static time_point_s parse_time_string(const std::string &time_str)
    {
        std::string_view s{time_str};
        s = s.substr(1, s.size() - 2);
        time_point_s local_time;
        std::istringstream iss{std::string(s)};
        iss >> std::chrono::parse("%Y-%m-%d_%H-%M-%S", local_time);
        return local_time;
    }

    static void set_time_zone(std::string zone)
    {
        time_zone_ = std::chrono::get_tzdb().locate_zone(zone);
    }

private:
    inline static const std::chrono::time_zone *time_zone_ = nullptr;
};
#endif // TEST_UTILS_HPP