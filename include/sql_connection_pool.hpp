#ifndef SQL_CONNECTION_POOL_HPP
#define SQL_CONNECTION_POOL_HPP

#include <stddef.h>
#include "mysql_connection.hpp"
#include "time_util.hpp"
#include "block_queue.hpp"
#include "time_util.hpp"
#include <mutex>
#include <string>

struct MySQLConnectionUptrDeleter // 智能指针的删除器 当智能指针析构时 自动将连接归还连接池
{
    void operator()(MySQLConnection *conn) const
    {
        if (conn)
        {
            conn->set_last_active_time(TimeUtil::now_sec()); // 更新时间戳
            SQLConnectionPool::getInstance()->put_connection(conn);
        }
    }
};

class SQLConnectionPool
{
public:
    SQLConnectionPool(const SQLConnectionPool &) = delete;
    SQLConnectionPool &operator=(const SQLConnectionPool &) = delete;
    SQLConnectionPool(SQLConnectionPool &&) = delete;
    SQLConnectionPool &operator=(SQLConnectionPool &&) = delete;
    static SQLConnectionPool *getInstance() // 懒汉单例 线程安全
    {
        static SQLConnectionPool pool;
        return &pool;
    }

    void init(size_t min_connections,
              size_t max_connections,
              uint32_t idle_timeout,
              uint32_t take_connection_timeout,
              std::string host,
              uint port,
              std::string user,
              std::string passwd,
              std::string db,
              const char *unix_socket,
              ulong client_flag);

    std::unique_ptr<MySQLConnection, MySQLConnectionUptrDeleter> get_connection();
    void put_connection(MySQLConnection *conn);
    void reclaim_idle_connections();
    void set_idle_timeout(uint32_t sec);
    size_t idle_count();
    void close_all_connections();
private:
    void new_connection_();
    SQLConnectionPool() = default;
    ~SQLConnectionPool() = default;

    size_t min_connections_;
    size_t max_connections_;
    Ms idle_timeout_;
    Ms take_connection_timeout_;

    std::mutex mtx_;
    BlockQueue<std::unique_ptr<MySQLConnection>> block_queue_;
    // 如果以 尾进头出的 队列的形式使用 频繁使用的总是被压入末尾 弹出的总是最久没用的 会导致连接的时间戳均匀更新
    // 以 尾进尾出的 栈的形式使用 这样弹出的连接总是最新压入的 如果使用压力不大 旧的连接就会超时 方便清理

    std::string host_;
    uint port_;
    std::string user_;
    std::string passwd_;
    std::string db_;
    const char *unix_socket_;
    ulong client_flag_;

    size_t idle_cnt_;
};

#endif // SQL_CONNECTION_POOL_HPP