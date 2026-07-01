#include "sql_connection_pool.hpp"

void SQLConnectionPool::init(size_t min_connections,
                             size_t max_connections,
                             uint32_t idle_timeout,
                             uint32_t take_connection_timeout,
                             std::string host,
                             uint port,
                             std::string user,
                             std::string passwd,
                             std::string db,
                             const char *unix_socket,
                             ulong client_flag)
{
    std::lock_guard<std::mutex> locker(mtx_);
    min_connections_ = min_connections;
    max_connections_ = max_connections;
    idle_timeout_ = Ms(idle_timeout);
    take_connection_timeout_ = Ms(take_connection_timeout);
    host_ = host;
    port_ = port;
    user_ = user;
    passwd_ = passwd;
    db = db_;
    unix_socket_ = unix_socket;
    client_flag_ = client_flag;
    for (size_t i = 0; i < min_connections_; i++)
    {
        new_connection_();
    }
    idle_cnt_ = min_connections_;
}

std::unique_ptr<MySQLConnection, MySQLConnectionUptrDeleter> SQLConnectionPool::get_connection()
{
    std::unique_ptr<MySQLConnection> uptr;
    if(block_queue_.pop_back(uptr, take_connection_timeout_))
    {
        return {uptr.release(), MySQLConnectionUptrDeleter{}};
    }
    return nullptr;
}

void SQLConnectionPool::new_connection_()
{
    std::unique_ptr<MySQLConnection> conn_uptr = std::make_unique<MySQLConnection>();
    conn_uptr->connect(host_, user_, passwd_, db_, port_, unix_socket_, client_flag_);
    block_queue_.push_back(std::move(conn_uptr), Ms(2000));
}

void SQLConnectionPool::put_connection(MySQLConnection *conn)
{
    block_queue_.push_back(std::unique_ptr<MySQLConnection>(conn), Ms(2000));
}

void SQLConnectionPool::reclaim_idle_connections()
{
    std::lock_guard<std::mutex> locker(mtx_);
    time_point_s now = TimeUtil::now_sec();
    while (block_queue_.size() > min_connections_)
    {
        if(now - block_queue_.front()->get_last_active_time() > idle_timeout_)
        {
            block_queue_.pop_front(Ms(2000));
        }
        else // 从头到尾一定是从旧到新 若检查到未超时 直接退出
        {
            break;
        }
    }
}
void SQLConnectionPool::set_idle_timeout(uint32_t msec)
{
    std::lock_guard<std::mutex> locker(mtx_);
    idle_timeout_ = Ms(msec);
}
size_t SQLConnectionPool::idle_count()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return idle_cnt_;
}
void SQLConnectionPool::close_all_connections()
{
    std::lock_guard<std::mutex> locker(mtx_);
    for(auto& conn_uptr : block_queue_)
    {
        conn_uptr->disconnect();
    }
}