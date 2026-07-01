#include "mysql_connection.hpp"
#include <iostream>
#include <string>
using namespace std;

int main()
{
    MySQLConnection conn;

    conn.connect("127.0.0.1", "root", "789456123EWQewq.", "yourdb", 3306, "", 0);

    const char *q = "SELECT username, passwd FROM user;";

    table t = conn.query(q);
    cout << "query...................\n";
    for (size_t i = 0; i < (*(t.begin())).second.size(); i++)
    {
        for (const auto &kv : t)
        {
            cout << kv.first << ":" << setw(10) << t[kv.first][i] << "    ";
        }
        cout << "\n";
    }

    constexpr const char *insert = "INSERT INTO user (username, passwd) VALUES('{}', '{}');";

    int sum = 0;
    for (size_t i = 0; i < 10; i++)
    {
        string e = format(insert, i, i + 10);
        sum += conn.execute(e);
    }
    cout << "sum = " << sum << '\n';
    t = conn.query(q);
    cout << "query...................\n";
    for (size_t i = 0; i < (*(t.begin())).second.size(); i++)
    {
        for (const auto &kv : t)
        {
            cout << kv.first << ":" << setw(10) << t[kv.first][i] << "    ";
        }
        cout << "\n";
    }

    constexpr const char *del = "DELETE FROM user WHERE username BETWEEN '0' AND '9';";
    cout << "sum = " << conn.execute(del) << '\n';
    t = conn.query(q);
    cout << "query...................\n";
    for (size_t i = 0; i < (*(t.begin())).second.size(); i++)
    {
        for (const auto &kv : t)
        {
            cout << kv.first << ":" << setw(10) << t[kv.first][i] << "    ";
        }
        cout << "\n";
    }
}