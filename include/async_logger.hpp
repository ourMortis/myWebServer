#ifndef ASYNC_LOGGER_HPP
#define ASYNC_LOGGER_HPP

#include "async_logger_config.hpp"

#include "block_queue.hpp"
#include "fixed_buffer.hpp"
#include "log_file.hpp"

#include <stddef.h>
#include <mutex>
#include <thread>
#include <assert.h>
#include <format>
#include <atomic>
#include <source_location>
#include <filesystem>
#include <vector>
#include <iostream>

using Ms = std::chrono::milliseconds;
using time_point_s = std::chrono::local_seconds;

class AsyncLogger
{
public:
    ~AsyncLogger() = default;
    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger &operator=(const AsyncLogger &) = delete;
    AsyncLogger(AsyncLogger &&) = delete;
    AsyncLogger &operator=(AsyncLogger &&) = delete;

    // 懒汉单例
    static AsyncLogger *getInstance()
    {
        static AsyncLogger logger;
        return &logger;
    }

    // 获取单例后init 如果是已经关闭的 返回true 否则返回false 这个函数在close后也可作为set函数使用
    bool init(std::string_view path = DEFAULT_LOG_PATH, // 日志存储文件夹路径
              std::string_view suffix = DEFAULT_LOG_SUFFIX, // 日志文件后缀 不会自动加'.' 应输入如".log"
              size_t current_buffer_size = CURRENT_BUFFER_SIZE, // 当前缓冲区大小 应当小于 max_file_size
              LogLevel log_level = DEFAULT_LOG_LEVEL, // 日志等级
              const uint32_t time_interval = ROLL_LOGFILE_TIME_INTERVAL_s, // 滚动文件时间间隔 当本次写入时间 减 上一次写入时间 大于此间隔 触发文件滚动
              const size_t max_file_size = MAX_LOGFILE_SIZE, // 日志文件最大字节数 保证不超过max(max_file_size, current_buffer_size)
              bool logfile_continuation = DEFAULT_LOGFILE_CONTINUATION, // 是否以续写模式打开 如果是 会接着最后一个日志文件续写 否则新建文件
              std::string_view time_zone = DEFAULT_TIME_ZONE); // 时区 如 "Asia/Shanghai"

    void start()
    {
        assert(initialized_ && is_close_);
        assert(fs_->is_open());
        is_close_ = false;
        write_thread_ = std::thread(&AsyncLogger::write_thread_func_, this);
    }

    void close()
    {
        assert(initialized_ && !is_close_);
        is_close_ = true;
        block_queue_.flush();
        write_thread_.join();
        fs_->flush();
    }

    bool is_close() const noexcept
    {
        return is_close_;
    }

    // 无写入长度检查, 使用时注意单条日志长度(包括自动补充的时间等信息的长度)不要超过STACK_BUFFER长度
    template <LogLevel lv, bool show_time = true, bool show_file = true, bool show_line = true, bool show_function = true, typename... Args>
    void log(std::source_location loc, const char *fmt, Args &&...args)
    {
        if (lv < log_level_ || is_close_)
        {
            return;
        }
        char stack_buffer[STACK_BUFFER_SIZE];
        char *start = stack_buffer;
        char *end = std::format_to(start, "[{}]",[]() constexpr -> const char * 
            {   switch (lv) // 就地格式化文件,行号,时间等信息, 直接写入stack_buffer
                {
                case LogLevel::DEBUG:
                    return "DEBUG";
                case LogLevel::INFO:
                    return "INFO ";
                case LogLevel::WARN:
                    return "WARN ";
                case LogLevel::ERROR:
                    return "ERROR";
                case LogLevel::FATAL:
                    return "FATAL";
                default:
                    return "UNKWN";} }());
        start = end;

        if constexpr (show_time)
        {
            auto now = time_zone_->to_local(std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()));
            end = std::format_to(start, "[{:%Y-%m-%d %H:%M:%S}]", now);
            start = end;
        }
        if constexpr (show_file)
        {
            end = std::format_to(start, "[{}]", loc.file_name());
            start = end;
        }
        if constexpr (show_line)
        {
            end = std::format_to(start, "[line:{}]", loc.line());
            start = end;
        }
        if constexpr (show_function)
        {
            end = std::format_to(start, "[{}]", loc.function_name());
            start = end;
        }
        end = std::vformat_to(start, fmt, std::make_format_args(std::forward<Args>(args)...)); // 就地格式化日志信息
        *(end++) = '\n';
        // 日志信息已经被格式化写入到stack buffer, 开始从stack buffer转移到current buffer
        std::lock_guard<std::mutex> locker(mtx_);
        size_t log_len = end - stack_buffer;
        if (current_buffer_->get_writable_size() < log_len)
        {  // 如果 current_buffer 写不下了, 就丢给阻塞队列, 换next_buffer
            block_queue_.push_back(std::move(current_buffer_), Ms(PRODUCER_TIMEOUT_ms));
            if (next_buffer_)
            {
                current_buffer_ = std::move(next_buffer_);
            }
            else //(极少发生) 如果此时next_buffer为空, new一块出来
            {
                current_buffer_ = std::make_unique<FixedBuffer>(CURRENT_BUFFER_SIZE, 0);
            }
        }
        current_buffer_->append(stack_buffer, log_len);
    }

private:
    AsyncLogger();

    void drop_(uint32_t number) // 丢弃number个缓冲区 打印错误+直接输出日志到文件
    {
        char stack_buffer[STACK_BUFFER_SIZE];
        auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        auto res = std::format_to_n(stack_buffer, STACK_BUFFER_SIZE,
                                    "Dropped log messages at {:%Y-%m-%d %H:%M:%S}, {} larger buffers\n", time_zone_->to_local(now), number);
        size_t len = res.size;
        std::cerr.write(stack_buffer, len);
        *fs_ << std::string_view(stack_buffer, len);
    }

    void write_thread_func_();

    // 文件
    std::unique_ptr<LogFile> fs_;

    // 设置相关成员 atomic保证线程安全
    std::atomic<bool> is_close_ = true;
    std::atomic<bool> initialized_ = false;
    std::atomic<LogLevel> log_level_;

    // 日志信息
    const std::chrono::time_zone *time_zone_;

    // buffer
    using BufferPtr = std::unique_ptr<FixedBuffer>;
    BufferPtr current_buffer_;
    size_t current_buffer_size_;
    BufferPtr next_buffer_;
    BlockQueue<BufferPtr> block_queue_;

    // 线程
    std::thread write_thread_;
    std::mutex mtx_;
};

inline void LOG_ALL(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::DEBUG, true, true, true, true>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

inline void LOG_DEBUG(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::DEBUG, true, true, true, false>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

inline void LOG_INFO(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::INFO, true, false, false, false>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

inline void LOG_WARN(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::WARN, true, false, false, false>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

inline void LOG_ERROR(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::ERROR, true, true, false, false>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

inline void LOG_FATAL(const char* fmt, auto&&... args)
{
    AsyncLogger::getInstance()->log<LogLevel::FATAL, true, true, false, false>(std::source_location::current(), fmt, std::forward<decltype(args)>(args)...);
}

#endif // ASYNC_LOGGER_HPP