#ifndef LOG_FILE_HPP
#define LOG_FILE_HPP

#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>
#include <assert.h>
#include "async_logger_config.hpp"

using time_point_s = std::chrono::local_seconds;
// 封装ofstream提供自动日志文件滚动功能 该类线程不安全
// 支持:
// 一定时间间隔后滚动 一定文件大小后滚动
// 自动命名日志文件 日志文件续写
class LogFile
{
public:
    LogFile(std::string_view path = DEFAULT_LOG_PATH,
        std::string_view suffix = DEFAULT_LOG_SUFFIX,
            const uint32_t time_interval = ROLL_LOGFILE_TIME_INTERVAL_s,
            const size_t max_file_size = MAX_LOGFILE_SIZE,
            bool logfile_continuation = DEFAULT_LOGFILE_CONTINUATION,
            std::string_view time_zone = DEFAULT_TIME_ZONE);
    ~LogFile();

    template <class T>
    LogFile &operator<<(const T &data)
    {
        roll_file_(data.size());
        fs_ << data;
        return *this;
    }

    void flush()
    {
        fs_.flush();
    }

    bool is_open() const noexcept
    {
        return fs_.is_open();
    }

    std::string get_current_logfile_path() const noexcept
    {
        return current_logfile_path_;
    }

    size_t get_current_file_size() const noexcept
    {
        return last_size_;
    }

    time_point_s get_last_roll_time() const noexcept
    {
        return last_time_;
    }

private:
    // 文件滚动策略:
    // 超时则新开文件 文件名时间部分更新 序号置1  [超时指的是当前函数调用时 距离上一次调用时的超时 而不是固定时间间隔]
    // 超大小则新开文件 文件名其他部分不动 序号上升 数据会被写到新的文件里 且 数据不会被截断
    // 可能出现要写入数据本身就大于要求的文件大小的情况, 但此时数据不会被截断, 而是完整写入新文件
    // 也就是说 新打开文件后 无论如何 数据都会被写入 尽管数据本身超出要求大小
    void roll_file_(size_t len);

    void open_lastlog2append_();

    inline std::string get_now_time_string_() const;

    inline std::filesystem::path set_logfile_path_(bool mode); // 根据成员变量 拼接成要使用的文件路径, mode:true超时, mode:false超大小

    inline time_point_s now_sec_() const;

    time_point_s parse_time_string_(std::string_view s) const;

    const std::chrono::seconds time_interval_;
    const size_t max_file_size_;

    const std::filesystem::path path_;
    std::string suffix_;
    size_t prefix_size_;

    std::string current_logfile_path_;
    time_point_s last_time_;
    size_t last_size_;
    uint32_t serial_number_;

    std::ofstream fs_;

    const std::chrono::time_zone *time_zone_;
};

#endif // LOG_FILE_HPP