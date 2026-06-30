#include "mysql_connection.hpp"
#include <iostream>
using namespace std;

int main()
{
    MySQLConnection conn;

    conn.connect("127.0.0.1", "root", "789456123EWQewq.", "yourdb", 3306, "", 0);

    const char *q = "SELECT username, passwd FROM user;";

    table t = conn.query(q);

    for (const auto &kv : t)
    {
        cout << "column name: " << setw(15) << kv.first << ":";
        for (const auto &v : kv.second)
        {
            cout << setw(20) << v;
        }
        cout << '\n';
    }
}