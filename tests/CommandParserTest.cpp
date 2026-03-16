#include <gtest/gtest.h>
#include "ClientHandler.h"

// ---- parseCommand ----

TEST(CommandParserTest, ParsesSetCommand) {
    Command cmd = parseCommand("SET name Kurt");
    EXPECT_EQ(cmd.name, "SET");
    ASSERT_EQ(cmd.args.size(), 2);
    EXPECT_EQ(cmd.args[0], "name");
    EXPECT_EQ(cmd.args[1], "Kurt");
}

TEST(CommandParserTest, ParsesGetCommand) {
    Command cmd = parseCommand("GET name");
    EXPECT_EQ(cmd.name, "GET");
    ASSERT_EQ(cmd.args.size(), 1);
    EXPECT_EQ(cmd.args[0], "name");
}

TEST(CommandParserTest, ParsesDelCommand) {
    Command cmd = parseCommand("DEL name");
    EXPECT_EQ(cmd.name, "DEL");
    ASSERT_EQ(cmd.args.size(), 1);
}

TEST(CommandParserTest, CommandNameIsUppercased) {
    Command cmd = parseCommand("set name Kurt");
    EXPECT_EQ(cmd.name, "SET");
}

TEST(CommandParserTest, MixedCaseCommandIsUppercased) {
    Command cmd = parseCommand("Set name Kurt");
    EXPECT_EQ(cmd.name, "SET");
}

TEST(CommandParserTest, ParsesExpireCommand) {
    Command cmd = parseCommand("EXPIRE session 30");
    EXPECT_EQ(cmd.name, "EXPIRE");
    ASSERT_EQ(cmd.args.size(), 2);
    EXPECT_EQ(cmd.args[0], "session");
    EXPECT_EQ(cmd.args[1], "30");
}

TEST(CommandParserTest, ParsesPingCommand) {
    Command cmd = parseCommand("PING");
    EXPECT_EQ(cmd.name, "PING");
    EXPECT_EQ(cmd.args.size(), 0);
}

// ---- executeCommand ----

TEST(ExecuteCommandTest, SetReturnsOk) {
    Store store;
    Command cmd = parseCommand("SET name Kurt");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, "+OK\r\n");
}

TEST(ExecuteCommandTest, GetReturnsValue) {
    Store store;
    store.set("name", "Kurt");
    Command cmd = parseCommand("GET name");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, "+Kurt\r\n");
}

TEST(ExecuteCommandTest, GetMissingKeyReturnsNil) {
    Store store;
    Command cmd = parseCommand("GET missing");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, "$-1\r\n");
}

TEST(ExecuteCommandTest, DelExistingKeyReturnsOne) {
    Store store;
    store.set("key", "value");
    Command cmd = parseCommand("DEL key");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, ":1\r\n");
}

TEST(ExecuteCommandTest, DelMissingKeyReturnsZero) {
    Store store;
    Command cmd = parseCommand("DEL missing");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, ":0\r\n");
}

TEST(ExecuteCommandTest, ExistsReturnOneForPresentKey) {
    Store store;
    store.set("key", "value");
    Command cmd = parseCommand("EXISTS key");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, ":1\r\n");
}

TEST(ExecuteCommandTest, ExistsReturnsZeroForMissingKey) {
    Store store;
    Command cmd = parseCommand("EXISTS missing");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, ":0\r\n");
}

TEST(ExecuteCommandTest, PingReturnsPong) {
    Store store;
    Command cmd = parseCommand("PING");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response, "+PONG\r\n");
}

TEST(ExecuteCommandTest, UnknownCommandReturnsError) {
    Store store;
    Command cmd = parseCommand("FLUSHALL");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response.substr(0, 4), "-ERR");
}

TEST(ExecuteCommandTest, SetMissingArgsReturnsError) {
    Store store;
    Command cmd = parseCommand("SET onlyonearg");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response.substr(0, 4), "-ERR");
}

TEST(ExecuteCommandTest, ExpireNonIntegerTTLReturnsError) {
    Store store;
    store.set("key", "value");
    Command cmd = parseCommand("EXPIRE key notanumber");
    std::string response = executeCommand(cmd, store);
    EXPECT_EQ(response.substr(0, 4), "-ERR");
}