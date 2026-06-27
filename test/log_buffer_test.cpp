#include "log_buffer.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

// 生成 count 个随机字符串
// 每个长度在 [min_len, max_len] 之间
inline std::vector<std::string> get_str(size_t min_len, size_t max_len, size_t count)
{
    static const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
    std::uniform_int_distribution<> char_dist(0, static_cast<int>(sizeof(charset)) - 2);

    std::vector<std::string> res;
    res.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        size_t len = len_dist(gen);
        std::string str;
        str.reserve(len);
        for (size_t j = 0; j < len; ++j)
        {
            str += charset[char_dist(gen)];
        }
        res.push_back(std::move(str));
    }
    return res;
}

std::string print_buffer_readable(Buffer& buffer)
{
    size_t len;
    const char* str = buffer.get_read_ptr(len);

    return std::string(str, str + len);
}

int test_string_num = 40;
int test_string_size_min = 0;
int test_string_size_max = 12;
int read_or_write_num = 20;

static std::random_device grd;
static std::mt19937 ggen(grd());
static std::uniform_int_distribution<> gdis(0, test_string_num - 1);

static std::random_device grd2;
static std::mt19937 ggen2(grd2());
static std::uniform_int_distribution<> gdis2(0, 3);

LogBuffer buffer1(120);
std::vector<std::string> strs;



int main()
{
    //获得随机字符串数组
    strs = get_str(test_string_size_min, test_string_size_max, test_string_num);
    //ans同步模拟读写后, 可读区的结果
    std::string ans = "";

    int yescnt = 0;
    for (int i = 0; i < read_or_write_num; i++)
    {
        //随机字符串下标, 随机选择读/写
        int idx = gdis(ggen);
        int flag = gdis2(ggen2);
        if(flag == 1 || flag == 2)
        {
            //写
            ans += strs[idx];
            buffer1.write(strs[idx].data(), strs[idx].size());
        }
        else if(flag == 3)
        {
            buffer1.clear();
            ans.clear();
        }
        else
        {
            //读
            auto tmp = buffer1.read(strs[idx].size());
            ans = ans.substr(tmp.size());
        }


        auto output = print_buffer_readable(buffer1);

        std::cout << i << " th test: \n buffer output:: " << output;
        std::cout << "\nans    output:: " << ans << "\n";
        if(output == ans)
        {
            yescnt++;
            std::cout << "==========Yes=========\n";
        }
        else
        {
            std::cout << "===========No=========\n";
        }
    }
    std::cout << "Yes / All: " << yescnt << " / " << read_or_write_num << "\n";
}