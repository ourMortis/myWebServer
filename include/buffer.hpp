#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "buffer_config.hpp"
#include <vector>
#include <assert.h>
#include <algorithm>

// 缓冲区大小不设上限, 只有 append() 会自动扩容

// [  prepandable   ] [  readable     ] [  writable    ] [
//                    |                 |                |
//                    read_pos          write_pos        size
class Buffer
{
public:
    Buffer(const size_t buffer_size = DEFAULT_BUFFER_SIZE, const size_t min_prependable_size = MIN_PREPENDABLE_SIZE) : buffer_(buffer_size), read_pos_(min_prependable_size), write_pos_(min_prependable_size)
    {
        assert(buffer_size > min_prependable_size);
    }
    ~Buffer() = default;

    // 为了对接 系统调用(readv 和 write) 提供返回 裸指针 和 可读/写长度 的读写接口
    const char *get_read_ptr(size_t &readable_len) const noexcept
    {
        readable_len = get_readable_size();
        return buffer_.data() + read_pos_;
    }
    char *get_write_ptr(size_t &writable_len) noexcept
    {
        writable_len = get_writable_size();
        return buffer_.data() + write_pos_;
    }

    // 读/写完成后必须调用set 更新pos成员变量
    void set_has_written(size_t len) noexcept
    {
        write_pos_ += len;
    }
    void set_has_read(size_t len) noexcept
    {
        read_pos_ += len;
    }

    void set_has_read() noexcept
    {
        read_pos_ = write_pos_;
    }

    // 将字符串接在缓冲区后, 如果不够, 会自动扩容
    void append(const char *str, size_t len)
    {
        assert(str);
        ensure_writable_(len);
        std::copy(str, str + len, buffer_.data() + write_pos_);
        // 不需要再在外部调用set
        set_has_written(len);
    }

    void clear() // 数据洗成0, 归位读写pos
    {
        fill(buffer_.begin(), buffer_.end(), 0);
        read_pos_ = MIN_PREPENDABLE_SIZE;
        write_pos_ = MIN_PREPENDABLE_SIZE;
    }

    void reset() noexcept // 不清洗数据, 归位读写pos
    {
        read_pos_ = MIN_PREPENDABLE_SIZE;
        write_pos_ = MIN_PREPENDABLE_SIZE;
    }

    size_t get_prependable_size() const noexcept // 最小值为MIN_PREPENDABLE_SIZE 一般随着read操作增大
    {
        return read_pos_;
    }
    size_t get_readable_size() const noexcept
    {
        return write_pos_ - read_pos_;
    }
    size_t get_writable_size() const noexcept
    {
        return buffer_.size() - write_pos_;
    }

protected: //提供给派生类访问
    const char *get_read_ptr_() noexcept
    {
        return buffer_.data() + read_pos_;
    }
    char *get_write_ptr_() noexcept
    {
        return buffer_.data() + write_pos_;
    }

private:
    void make_space_(size_t len);
    void ensure_writable_(size_t len);
    std::vector<char> buffer_;
    size_t read_pos_;
    size_t write_pos_;
};

#endif // BUFFER_HPP