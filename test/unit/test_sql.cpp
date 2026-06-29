#include <mysql/mysql.h>
#include <iostream>
int main()
{
    MYSQL *conn = mysql_init(nullptr);
    conn = mysql_real_connect(conn, "127.0.0.1", "root", "789456123EWQewq.", "yourdb", 3306, NULL, 0);

    if (!conn)
    {
        std::cout << "连接失败";
        return 0;
    }

    std::cout << "连接成功!\n";
    
    const char *q = "SELECT username, passwd FROM user;";

    if (mysql_query(conn, q) != 0)
    {
        std::cout << "查询失败";
        return 0;
    }
    auto m_result = mysql_store_result(conn);
    std::cout << "查询成功";

    auto rows = mysql_num_rows(m_result);
    auto cols = mysql_num_fields(m_result);

    std::cout << "row:" << rows << " clo:" << cols << '\n';

    for (int j = 0;j < rows;j++)
    {
        auto r_row = mysql_fetch_row(m_result);
        for (int i = 0; i < cols; i++)
        {
            std::cout << "th data: " << r_row[i] << ' ';
        }
        std::cout << '\n';
    }
}