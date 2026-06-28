#include "thread_pool.hpp"

ThreadPool::ThreadPool(size_t thread_count,
                       size_t max_task_count,
                       std::string_view thread_pool_name,
                       uint32_t thread_take_task_timeout,
                       uint32_t task_push_timeout) : thread_count_(thread_count),
                                                     max_task_count_(max_task_count),
                                                     thread_pool_name_(thread_pool_name),
                                                     thread_take_task_timeout_(thread_take_task_timeout),
                                                     task_push_timeout_(task_push_timeout),
                                                     pool_data_sptr_(std::make_shared<shared_pool_data>(shared_pool_data{BlockQueue<std::unique_ptr<Task>>(max_task_count), false, CloseMode::Graceful}))
{
    threads_.reserve(thread_count_);
    for (size_t i = 0; i < thread_count_; i++)
    {
        threads_.emplace_back(std::make_unique<std::thread>(&ThreadPool::worker, pool_data_sptr_));
    }
}

ThreadPool::~ThreadPool()
{
    set_close_mode(CloseMode::Fast);
    close();
}

void ThreadPool::set_close_mode(CloseMode mode) noexcept
{
    pool_data_sptr_->close_mode_ = mode;
}

bool ThreadPool::add_task(std::unique_ptr<Task> task_ptr)
{
    if (!task_ptr || pool_data_sptr_->is_close_)
        return false;
    return pool_data_sptr_->block_queue_.push_back(std::move(task_ptr), task_push_timeout_);
}

void ThreadPool::worker(std::shared_ptr<shared_pool_data> pool_data_sptr)
{
    while (!pool_data_sptr->is_close_ || pool_data_sptr->close_mode_ == CloseMode::Stop) // stop模式不退出循环
    {
        std::unique_ptr<Task> task_uptr; // 循环体内智能指针 每轮循环后释放资源
        bool is_take_task = pool_data_sptr->block_queue_.pop(task_uptr, thread_take_task_timeout_);
        if (is_take_task && task_uptr)
        {
            try // 捕获异常 防止因为某个线程出错导致整个程序崩溃
            {
                task_uptr->process();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Task run failed, exception: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Task run failed, unknown exception!" << std::endl;
            }
        }
    }
    if(pool_data_sptr->close_mode_ == CloseMode::Graceful)
    { // 优雅关闭模式 循环直到任务队列为空为止
        while (!pool_data_sptr->block_queue_.empty())
        {
            std::unique_ptr<Task> task_uptr; // 循环体内智能指针 每轮循环后释放资源
            bool is_take_task = pool_data_sptr->block_queue_.pop(task_uptr, thread_take_task_timeout_);
            if (is_take_task && task_uptr)
            {
                try // 捕获异常 防止因为某个线程出错导致整个程序崩溃
                {
                    task_uptr->process();
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Task run failed, exception: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Task run failed, unknown exception!" << std::endl;
                }
            }
        }
    } // 快速关闭模式直接退出即可 没有额外代码
}

void ThreadPool::close()
{
    pool_data_sptr_->is_close_ = true;
    last_closed_ = pool_data_sptr_->close_mode_;
    switch (pool_data_sptr_->close_mode_)
    {
    case CloseMode::Stop: // 关闭队列 阻止读写
        pool_data_sptr_->block_queue_.close();
        break;
    case CloseMode::Graceful: // 不关闭队列 正在入队的入完 队列中的全部读出
        break;
    case CloseMode::Fast: // 关闭队列 阻止读写
        pool_data_sptr_->block_queue_.close();
        break;
    default:
        break;
    }
}

void ThreadPool::start()
{
    if (!pool_data_sptr_->is_close_ && !threads_.empty()) // 只有关闭了 或者 线程池为空才能重启
    {
        return;
    }
    pool_data_sptr_->is_close_ = false;
    pool_data_sptr_->block_queue_.start();
    if(last_closed_ != CloseMode::Stop)
    {
        threads_.clear(); // 清空旧线程
        threads_.reserve(thread_count_); // 重新初始化
        for (size_t i = 0; i < thread_count_; i++)
        {
            threads_.emplace_back(std::make_unique<std::thread>(&ThreadPool::worker, this, pool_data_sptr_));
        }
    }

}
