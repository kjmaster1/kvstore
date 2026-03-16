#include "ClientHandler.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;

Command parseCommand(const string& line) {
    Command cmd;
    istringstream stream(line);
    string token;

    // First token is the command name — uppercase it for case-insensitivity
    if (stream >> token) {
        transform(token.begin(), token.end(), token.begin(), ::toupper);
        cmd.name = token;
    }

    // Remaining tokens are arguments
    while (stream >> token) {
        cmd.args.push_back(token);
    }

    return cmd;
}

string executeCommand(Command& cmd, Store& store) {
    if (cmd.name == "SET") {
        if (cmd.args.size() < 2) {
            return "-ERR wrong number of arguments for SET\r\n";
        }
        store.set(cmd.args[0], cmd.args[1]);
        return "+OK\r\n";
    }

    if (cmd.name == "GET") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for GET\r\n";
        }
        auto value = store.get(cmd.args[0]);
        if (!value.has_value()) {
            return "$-1\r\n";
        }
        return "+" + value.value() + "\r\n";
    }

    if (cmd.name == "DEL") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for DEL\r\n";
        }
        bool deleted = store.del(cmd.args[0]);
        return deleted ? ":1\r\n" : ":0\r\n";
    }

    if (cmd.name == "EXISTS") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for EXISTS\r\n";
        }
        bool found = store.exists(cmd.args[0]);
        return found ? ":1\r\n" : ":0\r\n";
    }

    if (cmd.name == "EXPIRE") {
        if (cmd.args.size() < 2) {
            return "-ERR wrong number of arguments for EXPIRE\r\n";
        }
        try {
            int ttl = stoi(cmd.args[1]);
            auto value = store.get(cmd.args[0]);
            if (!value.has_value()) {
                return ":0\r\n";
            }
            store.setWithExpiry(cmd.args[0], value.value(), ttl);
            return ":1\r\n";
        }
        catch (const invalid_argument&) {
            return "-ERR TTL must be an integer\r\n";
        }
    }

    if (cmd.name == "PING") {
        return "+PONG\r\n";
    }

    return "-ERR unknown command '" + cmd.name + "'\r\n";
}

void handleClient(SOCKET clientSocket, Store& store) {
    cout << "Client connected: " << clientSocket << endl;

    char buffer[1024];
    string accumulated;

    while (true) {
        // Read data from the socket into buffer
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead <= 0) {
            // 0 means client disconnected, negative means error
            cout << "Client disconnected: " << clientSocket << endl;
            break;
        }

        buffer[bytesRead] = '\0';
        accumulated += buffer;

        // Process all complete lines (terminated by \n)
        size_t pos;
        while ((pos = accumulated.find('\n')) != string::npos) {
            string line = accumulated.substr(0, pos);
            accumulated.erase(0, pos + 1);

            // Strip carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) continue;

            Command cmd = parseCommand(line);
            string response = executeCommand(cmd, store);

            send(clientSocket, response.c_str(), (int)response.size(), 0);
        }
    }

    closesocket(clientSocket);
}