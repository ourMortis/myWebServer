#include <format>
#include <iostream>

using namespace std;

int main()
{
    char buffer[1000];
    char* end = format_to(buffer, "{}[]{}", 1, 100);

    string_view s(buffer, end - buffer);

    cout << s << endl << end - buffer << endl;
}