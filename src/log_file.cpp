#include "log_file.hpp"

LogFile::LogFile(std::string_view path, std::string_view suffix,
                 const uint32_t time_interval, const size_t max_file_size,
                 bool logfile_continuation)
    : time_interval_(time_interval), max_file_size_(max_file_size),
      path_(path), suffix_(suffix), last_size_(0), serial_number_(1)
{
    assert(time_interval > 0);
    assert(max_file_size >= CURRENT_BUFFER_SIZE); // 如果最大文件size小于缓冲区的最大size 写入会导致超出大小
    std::filesystem::create_directories(path_);
    assert(std::filesystem::is_directory(path_));

    if (logfile_continuation)
    {
        open_lastlog2append_();
    }
    else
    {
        set_logfile_path_(true);
        last_time_ = TimeUtil::now_sec();
        fs_.open(current_logfile_path_);
        assert(fs_.is_open());
    }
    prefix_size_ = (std::filesystem::path(path_) / TimeUtil::now_sec_string()).string().size();
}
LogFile::~LogFile()
{
    fs_.flush();
    fs_.close();
}

void LogFile::roll_file_(size_t len)
{
    bool timeout = false;
    bool sizeout = false;
    time_point_s now = TimeUtil::now_sec();
    last_size_ += len;                      // 每次写入默认累加size
    if (now - last_time_ >= time_interval_) // 超时 更新last_time 序号置1
    {
        last_time_ = now;
        serial_number_ = 1;
        last_size_ = len; // 内容会写入新文件 所以last_size_更新为len
        timeout = true;
    }
    else if (last_size_ > max_file_size_) // 超大小和超时同时发生 只做超时的操作
    {
        last_size_ = len;
        serial_number_++;
        sizeout = true;
    }

    if (timeout)
    {
        fs_.flush();
        fs_.close();
        fs_.open(set_logfile_path_(true), std::ios::app);
    }
    else if (sizeout)
    {
        fs_.flush();
        fs_.close();
        fs_.open(set_logfile_path_(false), std::ios::app);
    }
    assert(fs_.is_open());
}

void LogFile::open_lastlog2append_()
{
    using namespace std::filesystem;
    std::string last_time_str;      // 最晚时间字符串 字符串的大小就是时间的大小
    uint32_t max_serial_number = 1; // 最大序号
    std::string last_filename;      // 最晚文件
    for (const auto &e : directory_iterator(path_))
    {
        std::string filename = e.path().filename().string();
        size_t idx1 = filename.find('[');
        size_t idx2 = filename.rfind(']');
        assert(idx1 != std::string::npos && idx2 != std::string::npos);
        std::string time_str = filename.substr(idx1, idx2 - idx1 + 1);
        size_t number = stoul(filename.substr(idx2 + 1));
        if (time_str > last_time_str) // 时间上 这个文件比目前最新的新
        {
            last_time_str = time_str;
            max_serial_number = number;
            last_filename = filename;
        }
        else if (time_str == last_time_str && number > max_serial_number) // 时间上相同 序号数字更新
        {
            max_serial_number = number;
            last_filename = filename;
        }
    }

    if (last_filename.empty()) // 没有文件就开新的
    {
        set_logfile_path_(true);
        last_time_ = TimeUtil::now_sec();
        fs_.open(current_logfile_path_);
        assert(fs_.is_open());
    }
    else
    {
        current_logfile_path_ = path(path_) / last_filename;
        last_time_ = TimeUtil::parse_time_string(last_time_str);
        serial_number_ = max_serial_number;
        last_size_ = file_size(current_logfile_path_);
        assert(last_size_ >= 0);
        if (fs_.is_open())
            fs_.close();
        fs_.open(current_logfile_path_, std::ios::app);
        assert(fs_.is_open());
    }
}

inline std::filesystem::path LogFile::set_logfile_path_(bool mode) // 根据成员变量 拼接成要使用的文件路径
{
    std::filesystem::path dir(path_);
    if (mode) // 因为超时 切换文件要更新文件名的时间部分, 序号默认为1
    {
        current_logfile_path_ = (dir / (TimeUtil::now_sec_string() + "1" + suffix_));
        return current_logfile_path_;
    }
    else // 因为未超时但超出大小, 文件名仅更新后半部分即可(序号上升, 前半部分长度固定)
    {
        return current_logfile_path_.replace(prefix_size_, std::string::npos, std::move(std::to_string(serial_number_) + suffix_));
    }
}