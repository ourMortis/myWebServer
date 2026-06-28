#include<chrono>
#include<iostream>
using namespace std;

using namespace std::chrono;

const auto *tz = get_tzdb().locate_zone("Asia/Shanghai");
inline std::string get_now_time_string_()
{
    using namespace std::chrono;
    auto now_sec = floor<seconds>(system_clock::now());
    auto local_time = tz->to_local(now_sec);
    return std::format("[{:%Y-%m-%d_%H-%M-%S}]", local_time);
}

// 必须保证传入的格式形如 [2026-06-26_05-03-37]
local_seconds parse_time_string_(const std::string &time_str)
{
    std::string_view s = time_str;
    s = s.substr(1, s.size() - 2);
    std::chrono::local_seconds local_tp;
    std::istringstream iss{std::string(s)};
    iss >> std::chrono::parse("%Y-%m-%d_%H-%M-%S", local_tp);
    return local_tp;
}
int main()
{
    auto zt = tz->to_local(floor<seconds>(system_clock::now()));

    cout << tz->name() << endl;
    cout << get_now_time_string_() << endl;
    cout << parse_time_string_(get_now_time_string_()) << endl;
}