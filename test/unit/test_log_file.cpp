#include "test_utils.hpp"
#include "log_file.hpp"
#include <thread>
#include <chrono>
#include <format>

using namespace std;
using time_point_s = std::chrono::local_seconds;

constexpr int WRITE_COUNT = 1000; // 总写入次数

constexpr int RANDOM_WAIT_TIME_RANGE_MIN_ms = 0; // 每次写入之后的等待时间
constexpr int RANDOM_WAIT_TIME_RANGE_MAX_ms = 180; // 总测试时间期望 = (max + min / 2) * write_count

constexpr int RANDOM_STR_SIZE_MIN = 0; // 随机字符串的长度最大值和最小值
constexpr int RANDOM_STR_SIZE_MAX = 26;
constexpr int STR_NUMBER = 100; // 共生成 number 个随机字符串 每次写入时从中随机选取一个
// 总测试写入的数据字节数期望是 (max + min / 2) * write_count

constexpr int START_MULTIPLY_FREQUENCY_ROUND = 750; // 从第r轮开始 等待时间变短
constexpr int MULTIPLICATION_FACTOR = 4; // 等待时间变为原来的 1/FACTOR
constexpr int REOPEN_EVERY_N_TIMES = 10; // 每times轮数 以续写的方式重新打开一个LogFile类 以验证续写功能

string path = "./log";
string suffix = ".txt";
int time_interval = 30;
int file_max_size = 120;

int main()
{
    int true_cnt = 0;

    vector<string> strs = TestUtils::get_str(RANDOM_STR_SIZE_MIN, RANDOM_STR_SIZE_MAX, STR_NUMBER); // 准备字符串
    //初始化
    chrono::seconds time_interval_s(time_interval);
    LogFile *lf = new LogFile(path, suffix, time_interval, file_max_size, false);
    time_point_s last_time = lf->get_last_roll_time();
    size_t last_size = 0;
    filesystem::path current_file_path = lf->get_current_logfile_path();
    TestUtils::set_time_zone(TIME_ZONE);
    // 开始验证
    for (int i = 1; i <= WRITE_COUNT; i++)
    {
        bool true_size = true;
        bool true_time = true;
        bool true_path = true;
        cout << i << "th: ";
        int time_ms = TestUtils::random(RANDOM_WAIT_TIME_RANGE_MIN_ms, RANDOM_WAIT_TIME_RANGE_MAX_ms); //随机等待时间
        if (i >= START_MULTIPLY_FREQUENCY_ROUND)
        {
            time_ms /= 4;
        }
        int index = TestUtils::random(0, STR_NUMBER - 1); // 随机取字符串
        if (i % REOPEN_EVERY_N_TIMES == 0) // 重开
        {
            delete lf;
            lf = new LogFile(path, suffix, time_interval, file_max_size, true);
        }
        int len = strs[index].size();
        (*lf) << strs[index];
        cout << "[write size]" << len; // 实际写入
        last_size += len;
        if (last_size > file_max_size) // 判断超大小
        {
            last_size = len;
        }
        if (current_file_path != lf->get_current_logfile_path()) // 判断是否实际发生roll
        {
            last_size = len; // 发生roll lastsize一定置为len
        }
        cout << " [true_size/file_size]" << last_size << "/" << lf->get_current_file_size();
        if (lf->get_current_file_size() != last_size) // 对比文件大小数值
        {
            true_size = false;
            last_size = lf->get_current_file_size(); // 如果不对 更新为正确的 防止干扰之后测试
        }
        if (current_file_path != lf->get_current_logfile_path()) // 如果发生roll
        {
            int idx1 = current_file_path.string().find('[');
            int idx2 = current_file_path.string().rfind(']');
            int idx3 = lf->get_current_logfile_path().find('[');
            int idx4 = lf->get_current_logfile_path().rfind(']');
            string timestr1 = current_file_path.string().substr(idx1, idx2 - idx1 + 1);
            string timestr2 = lf->get_current_logfile_path().substr(idx1, idx2 - idx1 + 1);
            time_point_s t1 = TestUtils::parse_time_string(timestr1);
            time_point_s t2 = TestUtils::parse_time_string(timestr2);

            if(t1 != t2) // 是因为超时发生roll 则新文件时间一定要相对于旧文件是超时的
            {
                if(t2 - t1 > time_interval_s)
                {
                    true_time = false;
                    true_path = false;
                }
            }
            else // 否则就是因为超出文件大小发生roll 此时序号一定上升
            {
                if(last_size == len 
                    && stoul(lf->get_current_logfile_path().substr(idx4+1)) != 
                    stoul(current_file_path.string().substr(idx2+1)) + 1)
                {
                    true_path = false;
                }
            }

            cout << " [rollfile old/new]\n" << current_file_path.string();
            current_file_path = lf->get_current_logfile_path(); // 更新为正确的path 防止干扰之后测试
            cout << "\n" << current_file_path.string() << "\n";
        }
        if (true_time && true_size && true_path)
        {
            true_cnt++;
        }
        cout << " [size]" << true_size << " [time]" << true_time << " [path]" << true_path << "\n";
        this_thread::sleep_for(chrono::milliseconds(time_ms));
    }

    cout << "yes/all = " << true_cnt << "/" << WRITE_COUNT << '\n';
}