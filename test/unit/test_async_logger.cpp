#include "async_logger.hpp"

#include "async_logger.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <filesystem>

// ========== 测试可调参数(小批量测试无需修改，注释标明调整场景) ==========
constexpr size_t TEST_THREAD_NUM = 4;  // 并发生产者线程数
constexpr size_t LOG_PER_THREAD = 250; // 每个线程打印日志条数
constexpr LogLevel TEST_LOG_LEVEL = LogLevel::DEBUG;
constexpr bool ENABLE_RAND_SLEEP = true; // true模拟业务间歇打日志，false高频压测缓冲区
// 单条日志最大长度限制：STACK_BUFFER_SIZE=512，日志整体(源码+时间+内容)不能超512字符
// 如需超长日志测试，修改 async_logger_config.hpp STACK_BUFFER_SIZE

// 生成随机日志文本，控制长度避免栈缓冲区溢出
std::string random_log_text()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<> len_range(8, 40);
    static std::uniform_int_distribution<> char_range(33, 122);

    std::string buf;
    size_t len = len_range(rng);
    buf.reserve(len);
    for (size_t i = 0; i < len; ++i)
    {
        buf += static_cast<char>(char_range(rng));
    }
    return buf;
}

// 生产者线程函数：多线程并发写入日志
void log_producer(int tid)
{
    AsyncLogger *logger = AsyncLogger::getInstance();
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> sleep_ms(1, 3);
    std::uniform_int_distribution<> lv_rand(0, 4); // DEBUG ~ FATAL

    for (size_t i = 0; i < LOG_PER_THREAD; ++i)
    {
        std::string content = random_log_text();
        int rand_lv = lv_rand(rng);

        switch (static_cast<LogLevel>(rand_lv))
        {
        case LogLevel::DEBUG:
            LOG_DEBUG("T{} LogIdx:{} Msg:{}", tid, i, content);
            break;
        case LogLevel::INFO:
            LOG_INFO("T{} LogIdx:{} Msg:{}", tid, i, content);
            break;
        case LogLevel::WARN:
            LOG_WARN("T{} LogIdx:{} Msg:{}", tid, i, content);
            break;
        case LogLevel::ERROR:
            LOG_ERROR("T{} LogIdx:{} Msg:{}", tid, i, content);
            break;
        case LogLevel::FATAL:
            LOG_FATAL("T{} LogIdx:{} Msg:{}", tid, i, content);
            break;
        default:
            break;
        }

        if constexpr (ENABLE_RAND_SLEEP)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms(rng)));
        }
    }
}

int main()
{
    AsyncLogger *logger = AsyncLogger::getInstance();

    // 日志输出配置：仅单文件输出，文件滚动功能未实现，max_line参数无实际作用
    const char *log_dir = "./single_log_test/";
    const char *suffix = ".log";
    const uint32_t dummy_max_line = 9999; // 切文件逻辑未实现，此参数仅占位，不会触发分割
    const size_t buf_size = CURRENT_BUFFER_SIZE;

    // 创建日志目录，避免打开文件失败
    std::filesystem::create_directories(log_dir);

    // 初始化日志器
    bool init_ok = logger->init(log_dir, suffix, buf_size, TEST_LOG_LEVEL);
    if (!init_ok)
    {
        std::cerr << "[ERROR] AsyncLogger init failed!" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "[INFO] Logger init success, single file mode (file rolling not implemented)" << std::endl;

    // 启动后端写线程
    logger->start();
    std::cout << "[INFO] Backend write thread started" << std::endl;

    // 多线程并发写入日志
    std::vector<std::thread> workers;
    workers.reserve(TEST_THREAD_NUM);
    std::cout << "[INFO] Launch " << TEST_THREAD_NUM << " producer threads, each write " << LOG_PER_THREAD << " logs" << std::endl;

    for (int i = 0; i < TEST_THREAD_NUM; ++i)
    {
        workers.emplace_back(log_producer, i);
    }

    // 等待所有生产者完成日志写入
    for (auto &t : workers)
    {
        t.join();
    }
    std::cout << "[INFO] All producer threads finished writing" << std::endl;

    // 关闭日志器：阻塞等待缓冲区全部落盘
    logger->close();
    std::cout << "[INFO] Logger closed, all buffer flushed to single log file" << std::endl;

    std::cout << "\nTest complete! Check log file under " << log_dir << std::endl;
    std::cout << "Verification points: " << std::endl;
    std::cout << "1. All logs exist in one single file (no split files)" << std::endl;
    std::cout << "2. Log format complete: [file:line func][LEVEL] time message" << std::endl;
    std::cout << "3. No garbled text, missing logs or repeated lines under multi-thread" << std::endl;
    std::cout << "4. Dropped buffer warning may appear if log speed exceeds write capacity" << std::endl;

    return EXIT_SUCCESS;
}
