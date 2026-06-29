#ifndef MYSQL_CONNECTION_HPP
#define MYSQL_CONNECTION_HPP

#include "mysql/mysql.h"
#include <chrono>
#include <string>
#include <string_view>

using time_point_s = std::chrono::local_seconds;

// 对MySQL连接进行封装
class MySQLConnection
{
public:
    MySQLConnection(); // RAII
    ~MySQLConnection();

    bool connect(MYSQL *mysql,
                 std::string_view host,
                 std::string_view user,
                 std::string_view passwd,
                 std::string_view db,
                 uint port,
                 std::string_view unix_socket,
                 ulong client_flag);
    bool update(std::string_view sql); // 任何修改行为 包括 插入 删除
    bool query(std::string_view sql);
private:
    void free_result_();
    MYSQL *mysql_connection_ = nullptr;
    MYSQL_RES *mysql_result_ = nullptr;
    MYSQL_ROW mysql_row_ = nullptr;
    time_point_s active_timestamp_;
};

#endif // MYSQL_CONNECTION_HPP