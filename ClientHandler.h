#pragma once

#include <string>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "Store.h"

// Represents a parsed command from the client
struct Command {
    std::string name;                // SET, GET, DEL, EXISTS, EXPIRE
    std::vector<std::string> args;   // the arguments after the command name
};

// Parse a raw input line into a Command
Command parseCommand(const std::string& line);

// Execute a parsed command against the store and return the response string
std::string executeCommand(Command& cmd, Store& store);

// Handle a single client connection — reads commands, writes responses
void handleClient(SOCKET clientSocket, Store& store);