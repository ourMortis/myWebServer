#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "block_queue.hpp"
#include "task.hpp"
#include <thread>
#include <atomic>
#include <string>
#include <string_view>
#include <cstddef>
#include <iostream>

using Ms = std::chrono::milliseconds;

enum class CloseMode { // 退出策略
    Stop, // 暂停 拒绝任务插入 拒绝线程取出任务 但线程依然运行
    Graceful, // 拒绝任务插入 等待当前任务队列全部执行完 退出线程 优雅关闭
    Fast // 等待线程执行完正在进行的任务 退出线程
    };

// 该类仅有add_task和emplace_task接口线程安全
class ThreadPool
{
public:
    explicit ThreadPool(size_t thread_count, size_t max_task_count,
                        std::string_view thread_pool_name,
                        uint32_t thread_take_task_timeout, uint32_t task_push_timeout);
    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    bool add_task(std::unique_ptr<Task> task_ptr);

    template <typename T, typename... Args>
    bool emplace_task(Args &&...args)
    {
        auto task_ptr = std::make_unique<T>(std::forward<Args>(args)...);
        return add_task(std::move(task_ptr));
    }

    void set_close_mode(CloseMode mode) noexcept;

    void close();

    void start();

    size_t get_thread_count() const noexcept
    {
        return thread_count_;
    }

    size_t get_max_task_count() const noexcept
    {
        return max_task_count_;
    }

    std::string_view get_name() const noexcept
    {
        return thread_pool_name_;
    }

private:
    struct shared_pool_data
    {
        BlockQueue<std::unique_ptr<Task>> block_queue_;
        std::atomic<bool> is_close_;
        std::atomic<CloseMode> close_mode_;
    };

    void worker(std::shared_ptr<shared_pool_data> pool_data_sptr);

    const size_t thread_count_;
    const size_t max_task_count_;
    const std::string thread_pool_name_;
    const Ms thread_take_task_timeout_;
    const Ms task_push_timeout_;
    CloseMode last_closed_;

    std::shared_ptr<shared_pool_data> pool_data_sptr_;
    std::vector<std::unique_ptr<std::thread>> threads_;
};

#endif // THREAD_POOL_HPP