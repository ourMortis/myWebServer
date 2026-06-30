#ifndef MYSQL_CONNECTION_HPP
#define MYSQL_CONNECTION_HPP

#include "mysql/mysql.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "time_util.hpp"

using table = std::unordered_map<std::string, std::vector<std::string>>; // 查询结果表格 对应 Table[列名][行号];

// 对MySQL连接进行封装
class MySQLConnection
{
public:
    MySQLConnection(); // RAII
    ~MySQLConnection();

    bool connect(std::string_view host,
                 std::string_view user,
                 std::string_view passwd,
                 std::string_view db,
                 uint port,
                 std::string_view unix_socket,
                 ulong client_flag);
    int execute(std::string_view sql); // 任何修改行为 包括 插入 删除 等 返回受影响的行数
    table query(std::string_view sql); // 查询返回结果
private:
    void update_timestamp_();
    MYSQL *mysql_connection_ = nullptr;
    time_point_s active_timestamp_;
};

#endif // MYSQL_CONNECTION_HPP