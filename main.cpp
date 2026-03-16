#include <iostream>
#include "Server.h"

using namespace std;

int main()
{
    try {
        Server server(6380);  // 6379 is the default Redis port
        server.run();
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}