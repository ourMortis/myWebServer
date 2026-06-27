#ifndef FIXED_BUFFER_HPP
#define FIXED_BUFFER_HPP

#include "buffer.hpp"
#include <string_view>

class FixedBuffer : public Buffer
{
public:
    FixedBuffer(const size_t buffer_size = DEFAULT_BUFFER_SIZE, const size_t min_prependable_size = MIN_PREPENDABLE_SIZE) :
    Buffer(buffer_size, min_prependable_size)
    {
        assert(buffer_size <= MAX_BUFFER_SIZE);
    }
    ~FixedBuffer() = default;

    std::string_view read(size_t len)
    {
        size_t read_len = 0;
        const char *ptr = get_read_ptr(read_len);
        read_len = std::min(len, read_len);
        set_has_read(read_len);
        return std::string_view{ptr, read_len};
    }
    std::string_view read()
    {
        size_t read_len = 0;
        const char *ptr = get_read_ptr(read_len);
        set_has_read(read_len);
        return std::string_view{ptr, read_len};
    }

    std::string_view peak(size_t len)
    {
        size_t read_len = 0;
        const char *ptr = get_read_ptr(read_len);
        read_len = std::min(len, read_len);
        return std::string_view{ptr, read_len};
    }
    std::string_view peak()
    {
        size_t read_len = 0;
        const char *ptr = get_read_ptr(read_len);
        return std::string_view{ptr, read_len};
    }

    // 不做安全检查, 使用前判断可写区域大小, 与基类相比, 不自动扩容
    inline void append_fix(const char* str, size_t len)
    {
        char* ptr = get_write_ptr_();
        std::copy(str, str + len, ptr);
        set_has_written(len);
    }
};

#endif // FIXED_BUFFER_HPP
