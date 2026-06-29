#include "thread_pool.hpp"
#include "task.hpp"
#include <iostream>
class MyTask : public Task
{
public:
    MyTask(int num)
    {
        a = num;
    }

    void process() override
    {
        char buf[64];
        auto id = std::this_thread::get_id();
        char* end = std::format_to(buf, "Tid:{} Task number:{} \n", id, a);
        std::cout.write(buf, end - buf);
        std::cout.flush();
    }

private:
    int a = 0;
};

int TEST_GRACEFUL_CNT = 100;
int TEST_FAST_CNT = 100;

int THREAD_CNT = 4;
int MAX_TASK_CNT = 15;

int main()
{
    ThreadPool tp(THREAD_CNT, MAX_TASK_CNT, "mypool", 1000, 1000);

    std::cout << "maxtaskcnt:" << tp.get_task_capacity() << '\n';
    std::cout << "task in queue:" << tp.get_task_count() << '\n';
    std::cout << "threadcnt:" << tp.get_thread_count() << '\n';
    std::cout << "pool_name:" << tp.get_name() << '\n';

    std::cout << "[Graceful] test cnt:" << TEST_GRACEFUL_CNT << "running..." << '\n';
    tp.set_close_mode(CloseMode::Graceful);
    tp.start();
    for (int i = 1; i <= TEST_GRACEFUL_CNT;i++)
    {
        tp.emplace_task<MyTask>(i);
    }
    tp.close();
    std::cout << "[Graceful] test end\n";
    std::cout << "task in queue:" << tp.get_task_count() << "\n\n";
    tp.clear_tasks();

    std::cout << "[Fast] test cnt:" << TEST_FAST_CNT << "running..." << '\n';
    tp.set_close_mode(CloseMode::Fast);
    tp.start();
    for (int i = 1; i <= TEST_FAST_CNT;i++)
    {
        tp.emplace_task<MyTask>(i);
    }
    tp.close();
    std::cout << "[Fast] test end\n";
    std::cout << "task in queue:" << tp.get_task_count() << "\n\n";
    tp.clear_tasks();
}