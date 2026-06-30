#ifndef ASYNC_LOGGER_CONFIG_HPP
#define ASYNC_LOGGER_CONFIG_HPP

#include <stddef.h>
#include <cstdint>

enum class LogLevel
{
    ALL,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    OFF
};

// 异步相关
constexpr size_t STACK_BUFFER_SIZE = 512; // 必须小于等于CURRENT_BUFFER_SIZE 这个大小也就是一条日志的最大长度(包括自动补充的时间等字符)
constexpr size_t CURRENT_BUFFER_SIZE = 512; // 当前缓冲区固定大小 每次写入文件的字节数不会超出这个大小 要求小于等于MAX_LOGFILE_SIZE
constexpr size_t MAX_BUFFERS_TO_WRITE_NUMBER = 8; // 批量写入的最多缓冲区数 要写入的缓冲区超出该数量 则认为生产过剩 丢弃所有缓冲区确保不会爆内存
constexpr uint32_t PRODUCER_TIMEOUT_ms = 1500; // 生产者线程的最长等待时间 超出时间则放弃当前缓冲区
constexpr uint32_t CONSUMER_TIMEOUT_ms = 3000; // 也就是写入的最小时间间隔 如果超出时间后缓冲区队列仍然空 仍然触发写入

// 文件相关
constexpr bool DEFAULT_LOGFILE_CONTINUATION = true; // 默认是否要自动打开路径下最后一个日志文件续写
constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO; // 默认日志等级
constexpr const char *DEFAULT_LOG_PATH = "./log"; // 默认日志文件存储路径
constexpr const char *DEFAULT_LOG_SUFFIX = ".lg"; // 默认后缀 不会自动加'.'
constexpr size_t MAX_LOGFILE_SIZE = 8000; // 默认日志文件最大大小 超出则会开新文件 数字指的是写入的字节数
constexpr uint32_t ROLL_LOGFILE_TIME_INTERVAL_s = 30; // 默认日志文件滚动时间间隔 超出后, 在下一次写入时开新文件

#endif // ASYNC_LOGGER_CONFIG_HPP