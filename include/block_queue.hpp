#ifndef BLOCK_QUEUE_HPP
#define BLOCK_QUEUE_HPP
#include <mutex>
#include <deque>
#include <condition_variable>
#include <chrono>
#include <assert.h>
#include <stddef.h>
#include <vector>

using Ms = std::chrono::milliseconds;

// 阻塞队列
// 支持生产者超时丢弃, 消费者限时等待
template <class T>
class BlockQueue
{
public:
    BlockQueue() = default;
    ~BlockQueue();

    void clear();
    bool empty();
    bool full();
    void close();

    bool wait_for_not_empty_or_timeout(Ms timeout);
    void swap_all(std::deque<T> &out);

    size_t size();
    size_t capacity();

    T front();
    T back();

    template <typename U>
    void push_back(U &&item, Ms timeout);

    bool pop(T &item, Ms timeout);
    void flush();

private:
    std::deque<T> deq_;
    size_t capacity_ = 100;
    std::mutex mtx_;
    bool is_close_ = false;
    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;
};

template <class T>
BlockQueue<T>::~BlockQueue()
{
    close();
};

template <class T>
void BlockQueue<T>::close()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        is_close_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
};

template <class T>
void BlockQueue<T>::flush()
{
    condConsumer_.notify_one();
};

template <class T>
void BlockQueue<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template <class T>
T BlockQueue<T>::front()
{
    std::lock_guard<std::mutex> locker(mtx_);
    assert(!deq_.empty());
    return deq_.front();
}

template <class T>
T BlockQueue<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    assert(!deq_.empty());
    return deq_.back();
}

template <class T>
size_t BlockQueue<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template <class T>
size_t BlockQueue<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template <class T>
bool BlockQueue<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template <class T>
bool BlockQueue<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

template <class T>
bool BlockQueue<T>::pop(T &item, Ms timeout)
{
    std::unique_lock<std::mutex> locker(mtx_);
    bool flag = condConsumer_.wait_for(locker, timeout, [this]()
                                       { return !deq_.empty() || is_close_; });
    if (!flag || is_close_)
    {
        return false;
    }
    item = std::move(deq_.front()); // 适配缓冲区, 转移所有权
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template <class T>
template <typename U>
void BlockQueue<T>::push_back(U &&item, Ms timeout)
{
    std::unique_lock<std::mutex> locker(mtx_);
    if(is_close_)
        return;
    bool flag = condProducer_.wait_for(locker, timeout, [this]()
                                       { return deq_.size() < capacity_ || is_close_; });
    if (!flag || is_close_)
        return;
    // 完美转发，原地移动/拷贝
    deq_.emplace_back(std::forward<U>(item));
    condConsumer_.notify_one();
}

template <class T>
void BlockQueue<T>::swap_all(std::deque<T> &out)
{
    std::lock_guard<std::mutex> lock(mtx_);
    deq_.swap(out);
    condProducer_.notify_one();
}

template <class T>
bool BlockQueue<T>::wait_for_not_empty_or_timeout(Ms timeout)
{
    std::unique_lock<std::mutex> lock(mtx_);
    // 等待：队列非空 || 关闭 || 超时
    return condConsumer_.wait_for(lock, timeout, [this]() {
        return !deq_.empty() || is_close_;
    });
}

#endif // BLOCK_QUEUE_HPP