#include "async_logger.hpp"
#include "test_utils.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <filesystem>
#include <chrono>

// 验证日志文件是否存在且非空
bool check_log_file_exists(const std::string& log_dir, const std::string& suffix) {
    namespace fs = std::filesystem;
    if (!fs::exists(log_dir)) {
        return false;
    }

    for (const auto& entry : fs::directory_iterator(log_dir)) {
        if (entry.path().extension() == suffix && fs::file_size(entry.path()) > 0) {
            return true;
        }
    }
    return false;
}

// 基础单线程日志写入测试
void test_basic_log_write() {
    // 初始化测试环境
    TestUtils::set_time_zone("Asia/Shanghai");
    const std::string log_path = "./test_logs_basic";
    const std::string log_suffix = ".log";
    
    // 初始化异步日志器
    auto logger = AsyncLogger::getInstance();
    bool init_ret = logger->init(log_path.c_str(), log_suffix.c_str(),
                                 4096, LogLevel::DEBUG, 30, 1024*1024, false, "Asia/Shanghai");
    assert(init_ret == true); // 首次初始化应为true
    
    // 启动日志线程
    logger->start();
    
    // 生成随机测试日志内容
    auto test_strs = TestUtils::get_str(10, 50, 10); // 生成10个随机字符串作为日志内容
    for (size_t i = 0; i < test_strs.size(); ++i) {
        LOG_DEBUG("Basic test log {}: {}", i, test_strs[i]);
        LOG_INFO("Info level log: {}", test_strs[i]);
        LOG_WARN("Warn level log: {}", test_strs[i]);
    }
    
    // 关闭日志器并刷盘
    logger->close();
    
    // 验证日志文件生成
    assert(check_log_file_exists(log_path, log_suffix) == true);
    std::cout << "[PASS] Basic log write test success!" << std::endl;

    // 清理测试文件
    std::filesystem::remove_all(log_path);
}

// 多线程并发写入测试
void test_multi_thread_log() {
    const std::string log_path = "./test_logs_multi";
    const std::string log_suffix = ".log";
    
    auto logger = AsyncLogger::getInstance();
    bool init_ret = logger->init(log_path.c_str(), log_suffix.c_str(),
                                 8192, LogLevel::DEBUG, 30, 2*1024*1024, false, "Asia/Shanghai");
    assert(init_ret == true);
    logger->start();

    // 启动10个线程并发写日志
    const size_t thread_num = 10;
    const size_t log_per_thread = 20;
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < thread_num; ++t) {
        threads.emplace_back([t, log_per_thread]() {
            auto thread_strs = TestUtils::get_str(8, 30, log_per_thread);
            for (size_t i = 0; i < log_per_thread; ++i) {
                LOG_ERROR("Thread {} log {}: {}", t, i, thread_strs[i]);
                // 随机休眠避免写入过快（可选）
                std::this_thread::sleep_for(std::chrono::microseconds(TestUtils::random<int>(10, 100)));
            }
        });
    }

    // 等待所有线程完成
    for (auto& th : threads) {
        th.join();
    }

    // 关闭日志器
    logger->close();
    
    // 验证日志文件非空
    assert(check_log_file_exists(log_path, log_suffix) == true);
    std::cout << "[PASS] Multi-thread log write test success!" << std::endl;

    // 清理测试文件
    std::filesystem::remove_all(log_path);
}

// 日志级别过滤测试
void test_log_level_filter() {
    const std::string log_path = "./test_logs_level";
    const std::string log_suffix = ".log";
    
    auto logger = AsyncLogger::getInstance();
    // 设置日志级别为ERROR，只应输出ERROR及FATAL级别日志
    bool init_ret = logger->init(log_path.c_str(), log_suffix.c_str(),
                                 4096, LogLevel::ERROR, 30, 1024*1024, false, "Asia/Shanghai");
    assert(init_ret == true);
    logger->start();

    // 尝试写入不同级别日志
    LOG_DEBUG("This debug log should be filtered"); // 应被过滤
    LOG_INFO("This info log should be filtered");   // 应被过滤
    LOG_WARN("This warn log should be filtered");   // 应被过滤
    LOG_ERROR("This error log should be saved");    // 应保留
    LOG_FATAL("This fatal log should be saved");    // 应保留

    logger->close();

    // 读取日志文件内容验证
    namespace fs = std::filesystem;
    bool has_error = false;
    bool has_fatal = false;
    bool has_debug = false;
    
    for (const auto& entry : fs::directory_iterator(log_path)) {
        if (entry.path().extension() == log_suffix) {
            std::ifstream file(entry.path());
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("[ERROR]") != std::string::npos) has_error = true;
                if (line.find("[FATAL]") != std::string::npos) has_fatal = true;
                if (line.find("[DEBUG]") != std::string::npos) has_debug = true;
            }
        }
    }

    assert(has_error == true && has_fatal == true && has_debug == false);
    std::cout << "[PASS] Log level filter test success!" << std::endl;

    // 清理测试文件
    std::filesystem::remove_all(log_path);
}

int main() {
    std::cout << "开始";
    try
    {
        test_basic_log_write();
        test_multi_thread_log();
        test_log_level_filter();
        std::cout << "All async logger tests passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Test failed: unknown exception" << std::endl;
        return 1;
    }
    return 0;
}