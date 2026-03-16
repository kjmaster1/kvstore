#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <atomic>
#include "Store.h"

class Server {
public:
    Server(int port);
    ~Server();
    void run();

    // Called by the signal handler to request shutdown
    void stop();

private:
    int port;
    SOCKET serverSocket;
    Store store;

    // atomic<bool> is thread-safe without a mutex
    // volatile alone is not sufficient for cross-thread signalling in C++
    std::atomic<bool> running;

    void initWinsock();
};