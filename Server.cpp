#include "Server.h"
#include "ClientHandler.h"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <csignal>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

using namespace std;

// Global pointer to the server instance so the signal handler can reach it
// This is one of the few legitimate uses of a global variable in C++
static Server* globalServer = nullptr;

static void signalHandler(int signal) {
    cout << "\nShutting down gracefully..." << endl;
    if (globalServer) {
        globalServer->stop();
    }
}

Server::Server(int port) : port(port), serverSocket(INVALID_SOCKET), running(false) {
    initWinsock();
    globalServer = this;

    // Register our signal handler for Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
}

Server::~Server() {
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    globalServer = nullptr;
}

void Server::stop() {
    running = false;
    // Close the server socket to unblock accept()
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

void Server::initWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw runtime_error("WSAStartup failed: " + to_string(result));
    }
#endif
}

void Server::run() {
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        throw runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) != 0) {
        throw runtime_error("Bind failed");
    }

    if (listen(serverSocket, 10) != 0) {
        throw runtime_error("Listen failed");
    }

    running = true;
    cout << "kvstore listening on port " << port << endl;
    cout << "Press Ctrl+C to stop" << endl;

    while (running) {
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);

        // If accept fails and we're shutting down, exit cleanly
        if (clientSocket == INVALID_SOCKET) {
            if (!running) {
                break;
            }
            cerr << "Accept failed" << endl;
            continue;
        }

        thread clientThread(handleClient, clientSocket, ref(store));
        clientThread.detach();
    }

    cout << "Server stopped." << endl;
}