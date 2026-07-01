#include "mysql_connection.hpp"

MySQLConnection::MySQLConnection() : mysql_connection_(mysql_init(nullptr))
{
}

MySQLConnection::~MySQLConnection()
{
    if (mysql_connection_ != nullptr)
    {
        mysql_close(mysql_connection_);
    }
}

bool MySQLConnection::connect(std::string_view host,
                              std::string_view user,
                              std::string_view passwd,
                              std::string_view db,
                              uint port,
                              std::string_view unix_socket,
                              ulong client_flag)
{
    const char *u_s = unix_socket == "" ? NULL : unix_socket.data();
    mysql_connection_ = mysql_real_connect(mysql_connection_,
                                           host.data(),
                                           user.data(),
                                           passwd.data(),
                                           db.data(),
                                           port,
                                           u_s,
                                           client_flag);
    return mysql_connection_ != nullptr;
}

int MySQLConnection::execute(std::string_view sql)
{
    if (mysql_query(mysql_connection_, sql.data()) != 0)
    {
        // 错误处理
        return -1;
    }
    uint64_t affected = mysql_affected_rows(mysql_connection_);
    return static_cast<int>(affected);
}

table MySQLConnection::query(std::string_view sql)
{
    if (mysql_query(mysql_connection_, sql.data()))
    {
        return {};
    }
    MYSQL_RES *sql_res = mysql_store_result(mysql_connection_);
    if (!sql_res)
    {
        return {};
    }
    uint n_cols = mysql_num_fields(sql_res);
    MYSQL_FIELD *fields = mysql_fetch_field(sql_res);
    table t;
    MYSQL_ROW row;
    for (size_t i = 0; i < n_cols; i++)
    {
        t[fields[i].name].reserve(n_cols);
    }
    while (row = mysql_fetch_row(sql_res))
    {
        for (size_t i = 0; i < n_cols; i++)
        {
            t[fields[i].name].emplace_back<std::string>(row[i] ? row[i] : "");
        }
    }
    mysql_free_result(sql_res);
    return t;
}

void MySQLConnection::set_last_active_time(time_point_s time)
{
    last_active_ = time;
}

time_point_s MySQLConnection::get_last_active_time()
{
    return last_active_;
}

void  MySQLConnection::disconnect()
{
    if(mysql_connection_ != nullptr)
    {
        mysql_close(mysql_connection_);
    }
}
bool MySQLConnection::is_connected()
{
    return mysql_connection_ != nullptr;
}