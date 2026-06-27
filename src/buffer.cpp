#include "buffer.hpp"

void Buffer::ensure_writable_(size_t len)
{
    if(buffer_.size() - write_pos_ < len)
    {
        make_space_(len);
    }
    assert(get_writable_size() >= len);
}

void Buffer::make_space_(size_t len)
{
    if(read_pos_ - MIN_PREPENDABLE_SIZE + get_writable_size() < len)
    {
        assert(write_pos_ + len + 1 <= MAX_BUFFER_SIZE);
        buffer_.resize(write_pos_ + len + 1);
    }
    else
    {
        const auto it = buffer_.begin();
        const size_t readable_size = get_readable_size();
        std::copy(it + read_pos_, it + write_pos_, it + MIN_PREPENDABLE_SIZE);
        read_pos_ = MIN_PREPENDABLE_SIZE;
        write_pos_ = read_pos_ + readable_size;
    }
}