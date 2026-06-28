#include "block_queue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

//AI生成


// 测试1：基本生产消费（单线程）
void test_basic()
{
    std::cout << "===== 测试1：基本生产消费 =====" << std::endl;
    BlockQueue<int> q(5);

    // 生产数据
    q.push_back(1);
    q.push_front(2);
    q.push_back(3);

    // 检查基本属性
    std::cout << "队列大小: " << q.size() << " (预期:3)" << std::endl;
    std::cout << "队首元素: " << q.front() << " (预期:2)" << std::endl;
    std::cout << "队尾元素: " << q.back() << " (预期:3)" << std::endl;

    // 消费数据
    int item;
    bool ret = q.pop(item);
    std::cout << "消费结果: " << ret << ", 消费值: " << item << " (预期:true,2)" << std::endl;
    ret = q.pop(item);
    std::cout << "消费结果: " << ret << ", 消费值: " << item << " (预期:true,1)" << std::endl;

    std::cout << "队列是否为空: " << q.empty() << " (预期:false)" << std::endl;
    q.clear();
    std::cout << "清空后队列是否为空: " << q.empty() << " (预期:true)" << std::endl;

    std::cout << std::endl;
}

// 测试2：多线程生产者+消费者
void test_multi_thread()
{
    std::cout << "===== 测试2：多线程生产消费 =====" << std::endl;
    BlockQueue<int> q(100);
    std::atomic<int> produce_count(0);
    std::atomic<int> consume_count(0);
    const int PRODUCE_TOTAL = 1000;
    const int CONSUME_TOTAL = 1000;

    // 生产者线程（2个）
    auto producer = [&](int id) {
        for (int i = 0; i < PRODUCE_TOTAL / 2; ++i) {
            q.push_back(id * 1000 + i);
            produce_count++;
            std::this_thread::sleep_for(Ms(1)); // 模拟生产耗时
        }
    };

    // 消费者线程（2个）
    auto consumer = [&](int id) {
        int item;
        while (consume_count < CONSUME_TOTAL) {
            if (q.pop(item, Ms(100))) { // 超时100ms
                consume_count++;
                // 验证数据有效性（可选）
                (void)item;
            }
        }
    };

    // 启动线程
    std::thread p1(producer, 1);
    std::thread p2(producer, 2);
    std::thread c1(consumer, 1);
    std::thread c2(consumer, 2);

    // 等待线程结束
    p1.join();
    p2.join();
    c1.join();
    c2.join();

    std::cout << "总生产数: " << produce_count << " (预期:1000)" << std::endl;
    std::cout << "总消费数: " << consume_count << " (预期:1000)" << std::endl;
    std::cout << "队列剩余元素: " << q.size() << " (预期:0)" << std::endl;

    std::cout << std::endl;
}

// 测试3：生产者超时（队列满）
void test_producer_timeout()
{
    std::cout << "===== 测试3：生产者超时丢弃 =====" << std::endl;
    BlockQueue<int> q(2); // 队列容量仅2

    // 先填满队列
    q.push_back(1);
    q.push_back(2);
    std::cout << "队列是否满: " << q.full() << " (预期:true)" << std::endl;

    // 尝试生产（超时100ms）
    auto start = std::chrono::steady_clock::now();
    q.push_back(3, Ms(100)); // 队列满，等待100ms后丢弃
    auto end = std::chrono::steady_clock::now();
    auto cost = std::chrono::duration_cast<Ms>(end - start).count();

    std::cout << "生产超时耗时: " << cost << "ms (预期≈100ms)" << std::endl;
    std::cout << "队列大小: " << q.size() << " (预期:2)" << std::endl;

    std::cout << std::endl;
}

// 测试4：消费者超时（队列为空）
void test_consumer_timeout()
{
    std::cout << "===== 测试4：消费者超时等待 =====" << std::endl;
    BlockQueue<int> q(5); // 空队列

    int item;
    auto start = std::chrono::steady_clock::now();
    bool ret = q.pop(item, Ms(200)); // 空队列，等待200ms超时
    auto end = std::chrono::steady_clock::now();
    auto cost = std::chrono::duration_cast<Ms>(end - start).count();

    std::cout << "消费结果: " << ret << " (预期:false)" << std::endl;
    std::cout << "消费超时耗时: " << cost << "ms (预期≈200ms)" << std::endl;

    std::cout << std::endl;
}

// 测试5：关闭队列后生产/消费
void test_close_queue()
{
    std::cout << "===== 测试5：关闭队列 =====" << std::endl;
    BlockQueue<int> q(5);
    q.push_back(10);

    // 关闭队列
    q.Close();

    // 尝试生产
    q.push_back(20);
    std::cout << "关闭后生产，队列大小: " << q.size() << " (预期:0，因为Close会clear)" << std::endl;

    // 尝试消费
    int item;
    bool ret = q.pop(item);
    std::cout << "关闭后消费结果: " << ret << " (预期:false)" << std::endl;

    std::cout << std::endl;
}

int main()
{
    try {
        test_basic();
        test_multi_thread();
        test_producer_timeout();
        test_consumer_timeout();
        test_close_queue();
        std::cout << "所有测试完成！" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}