#include "async_logger.hpp"

AsyncLogger::AsyncLogger() = default;

bool AsyncLogger::init(
    std::string_view path,
    std::string_view suffix,
    size_t current_buffer_size,
    LogLevel log_level,
    const uint32_t time_interval,
    const size_t max_file_size,
    bool logfile_continuation,
    std::string_view time_zone)
{
    if (is_close_)
    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (is_close_)
        {
            // 检查栈上缓存大小必须小于等于current_buffer大小
            assert(STACK_BUFFER_SIZE <= CURRENT_BUFFER_SIZE);
            log_level_ = log_level;
            current_buffer_size_ = current_buffer_size;
            time_zone_ = std::chrono::get_tzdb().locate_zone(time_zone);
            // 如果之前持有对象 自动析构
            current_buffer_ = std::make_unique<FixedBuffer>(current_buffer_size, 0);
            next_buffer_ = std::make_unique<FixedBuffer>(current_buffer_size, 0);
            fs_ = std::make_unique<LogFile>(path, suffix, time_interval, max_file_size, logfile_continuation, time_zone);

            assert(fs_->is_open());
            initialized_ = true;
            return true;
        }
    }
    return false;
}

void AsyncLogger::write_thread_func_()
{

    BufferPtr new_buf1 = std::make_unique<FixedBuffer>(CURRENT_BUFFER_SIZE, 0);
    BufferPtr new_buf2 = std::make_unique<FixedBuffer>(CURRENT_BUFFER_SIZE, 0);
    std::deque<BufferPtr> local_buffers; // 用于交换的队列

    while (!is_close_)
    {
        assert(new_buf1 && new_buf1->get_readable_size() == 0);
        assert(new_buf2 && new_buf2->get_readable_size() == 0);
        assert(local_buffers.empty());

        block_queue_.wait_for_not_empty_or_timeout(Ms(CONSUMER_TIMEOUT_ms));
        // 超时或获得缓冲区 将当前队列内和current_buffer全部移入buffers_to_write
        { // 线程安全的交换
            std::lock_guard<std::mutex> lock(mtx_);
            block_queue_.swap_all(local_buffers);
            local_buffers.push_back(std::move(current_buffer_));
            current_buffer_ = std::move(new_buf1);
            if (!next_buffer_)
                next_buffer_ = std::move(new_buf2);
        }

        assert(!local_buffers.empty());

        if (local_buffers.size() > MAX_BUFFERS_TO_WRITE_NUMBER) // 缓冲区过多, 写入来不及会爆内存
        {
            drop_(local_buffers.size() - 2); // 保留两个丢弃
            local_buffers.erase(local_buffers.begin() + 2, local_buffers.end());
        }
        for (const auto &buffer : local_buffers)
            *fs_ << buffer->peak(); // 本地批量循环写入
        fs_->flush();               // 写完 flush
        if (local_buffers.size() > 2)
            local_buffers.resize(2); // 保留两个缓冲区

        // 从local_buffers中弹出一个作为new_buf1
        // 运行到此处时 new_buf1已经转移到current_buffer( current_buffer_ = std::move(new_buf1);
        // local_buffers 也必然有之前压入的 current_buffer( local_buffers.push_back(std::move(current_buffer_));
        assert(!local_buffers.empty());
        new_buf1 = std::move(local_buffers.back());
        local_buffers.pop_back();
        new_buf1->reset();

        // 从local_buffers中弹出一个作为new_buf2
        // 如果new_buf2为空 说明next_buffer之前为空( if (!next_buffer_) next_buffer_ = std::move(new_buf2);
        // 这种情况只能是因为生产者线程触发的: 将current_buffer入队 + next_buffer作为新的current_buffer
        // 此时程序因为触发了队列非空走到这里, 队列已有一个buffer + 前面代码强制压入的 current_buffer(也就是之前的next_buffer)
        // local_buffers 有两个buffer, 上面代码弹出了一个 到这里还剩一个
        if (!new_buf2)
        {
            assert(!local_buffers.empty()); // 能运行到这, 肯定还有一个buffer
            new_buf2 = std::move(local_buffers.back());
            local_buffers.pop_back();
            new_buf2->reset();
        }
        // 清空 local_buffers 用作下次swap
        local_buffers.clear();
    }
    fs_->flush();
}