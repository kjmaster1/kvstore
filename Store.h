#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <chrono>

class Store {
public:
    // Set a key to a value
    void set(const std::string& key, const std::string& value);

    // Set a key with a TTL (time to live) in seconds
    void setWithExpiry(const std::string& key, const std::string& value, int ttlSeconds);

    // Get a value by key — returns empty optional if not found or expired
    std::optional<std::string> get(const std::string& key);

    // Delete a key — returns true if it existed
    bool del(const std::string& key);

    // Check if a key exists and has not expired
    bool exists(const std::string& key);

private:
    // The actual data store
    std::unordered_map<std::string, std::string> data;

    // Expiry times — key maps to the time_point when it expires
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiries;

    // The mutex — protects data and expiries from concurrent access
    std::mutex mtx;

    // Internal helper — checks if a key has expired and removes it if so
    // Must be called while holding the mutex
    bool isExpired(const std::string& key);
};