#include "mysql_connection.hpp"

MySQLConnection::MySQLConnection()
{
    mysql_connection_ = mysql_init(nullptr);
}

MySQLConnection::~MySQLConnection()
{
    mysql_free_result(mysql_result_);
    if(mysql_connection_ != nullptr)
    {
        mysql_close(mysql_connection_);
    }
}

bool MySQLConnection::connect(MYSQL *mysql,
                              std::string_view host,
                              std::string_view user,
                              std::string_view passwd,
                              std::string_view db,
                              uint port,
                              std::string_view unix_socket,
                              ulong client_flag)
{
    mysql_connection_ = mysql_real_connect(mysql_connection_,
                                           host.data(),
                                           user.data(),
                                           passwd.data(),
                                           db.data(),
                                           port,
                                           unix_socket.data(),
                                           client_flag);
    return mysql_connection_ != nullptr;
}

bool MySQLConnection::update(std::string_view sql)
{
    return mysql_query(mysql_connection_, sql.data()) == 0;
}

bool MySQLConnection::query(std::string_view sql)
{
    free_result_();
    if(mysql_query(mysql_connection_, sql.data()))
    {
        return false;
    }
    mysql_result_ = mysql_store_result(mysql_connection_);
    return true;
}

void MySQLConnection::free_result_()
{
    if(mysql_result_ != nullptr)
    {
        mysql_free_result(mysql_result_);
        mysql_result_ = nullptr;
    }
}