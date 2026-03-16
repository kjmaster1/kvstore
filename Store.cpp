#include "Store.h"

using namespace std;
using namespace std::chrono;

void Store::set(const string& key, const string& value) {
    lock_guard<mutex> lock(mtx);
    data[key] = value;
    // Clear any existing expiry when a key is overwritten
    expiries.erase(key);
}

void Store::setWithExpiry(const string& key, const string& value, int ttlSeconds) {
    lock_guard<mutex> lock(mtx);
    data[key] = value;
    expiries[key] = steady_clock::now() + seconds(ttlSeconds);
}

optional<string> Store::get(const string& key) {
    lock_guard<mutex> lock(mtx);
    if (isExpired(key)) {
        return nullopt;
    }
    auto it = data.find(key);
    if (it == data.end()) {
        return nullopt;
    }
    return it->second;
}

bool Store::del(const string& key) {
    lock_guard<mutex> lock(mtx);
    expiries.erase(key);
    return data.erase(key) > 0;
}

bool Store::exists(const string& key) {
    lock_guard<mutex> lock(mtx);
    if (isExpired(key)) {
        return false;
    }
    return data.count(key) > 0;
}

bool Store::isExpired(const string& key) {
    auto it = expiries.find(key);
    if (it == expiries.end()) {
        return false;
    }
    if (steady_clock::now() >= it->second) {
        data.erase(key);
        expiries.erase(it);
        return true;
    }
    return false;
}