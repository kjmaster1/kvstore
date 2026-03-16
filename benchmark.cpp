#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <stdexcept>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;
using namespace std::chrono;

// Simple synchronous client for benchmarking
class BenchmarkClient {
public:
    BenchmarkClient(const string& host, int port) {
#ifdef _WIN32
        WSADATA wsaData;
        int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (wsaResult != 0) {
            throw runtime_error("WSAStartup failed");
        }
#endif
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            throw runtime_error("Failed to create socket");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
            throw runtime_error("Failed to connect to server");
        }
    }

    ~BenchmarkClient() {
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Send a command and read the response
    string sendCommand(const string& cmd) {
        string message = cmd + "\r\n";
        send(sock, message.c_str(), (int)message.size(), 0);

        char buffer[256];
        int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            throw runtime_error("Connection closed");
        }
        buffer[bytesRead] = '\0';
        return string(buffer);
    }

private:
    SOCKET sock;
};

void runBenchmark(const string& name, int operations, auto benchmarkFn) {
    auto start = steady_clock::now();

    for (int i = 0; i < operations; i++) {
        benchmarkFn(i);
    }

    auto end = steady_clock::now();
    double seconds = duration<double>(end - start).count();
    double opsPerSecond = operations / seconds;

    cout << name << ":" << endl;
    cout << "  " << operations << " operations in "
        << fixed << setprecision(3) << seconds << "s" << endl;
    cout << "  " << (int)opsPerSecond << " ops/sec" << endl;
    cout << endl;
}

int main() {
    const int OPERATIONS = 10000;
    const string HOST = "127.0.0.1";
    const int PORT = 6380;

    cout << "kvstore benchmark" << endl;
    cout << "Operations per test: " << OPERATIONS << endl;
    cout << "Connecting to " << HOST << ":" << PORT << endl;
    cout << endl;

    try {
        // SET benchmark
        {
            BenchmarkClient client(HOST, PORT);
            runBenchmark("SET", OPERATIONS, [&](int i) {
                client.sendCommand("SET key" + to_string(i) + " value" + to_string(i));
                });
        }

        // GET benchmark
        {
            BenchmarkClient client(HOST, PORT);
            runBenchmark("GET", OPERATIONS, [&](int i) {
                client.sendCommand("GET key" + to_string(i));
                });
        }

        // DEL benchmark
        {
            BenchmarkClient client(HOST, PORT);
            runBenchmark("DEL", OPERATIONS, [&](int i) {
                client.sendCommand("DEL key" + to_string(i));
                });
        }

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "Benchmark complete." << endl;
    return 0;
}