#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "Store.h"

using namespace std::chrono_literals;

// ---- SET and GET ----

TEST(StoreTest, SetAndGetValue) {
    Store store;
    store.set("name", "Kurt");
    auto result = store.get("name");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Kurt");
}

TEST(StoreTest, GetMissingKeyReturnsNullopt) {
    Store store;
    auto result = store.get("missing");
    EXPECT_FALSE(result.has_value());
}

TEST(StoreTest, SetOverwritesExistingValue) {
    Store store;
    store.set("key", "first");
    store.set("key", "second");
    auto result = store.get("key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "second");
}

// ---- EXISTS ----

TEST(StoreTest, ExistsReturnsTrueForPresentKey) {
    Store store;
    store.set("key", "value");
    EXPECT_TRUE(store.exists("key"));
}

TEST(StoreTest, ExistsReturnsFalseForMissingKey) {
    Store store;
    EXPECT_FALSE(store.exists("missing"));
}

// ---- DEL ----

TEST(StoreTest, DelReturnsTrueAndRemovesKey) {
    Store store;
    store.set("key", "value");
    bool deleted = store.del("key");
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(store.exists("key"));
}

TEST(StoreTest, DelReturnsFalseForMissingKey) {
    Store store;
    bool deleted = store.del("missing");
    EXPECT_FALSE(deleted);
}

TEST(StoreTest, GetAfterDelReturnsNullopt) {
    Store store;
    store.set("key", "value");
    store.del("key");
    auto result = store.get("key");
    EXPECT_FALSE(result.has_value());
}

// ---- TTL / EXPIRY ----

TEST(StoreTest, KeyExistsBeforeExpiry) {
    Store store;
    store.setWithExpiry("session", "abc123", 2);
    auto result = store.get("session");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "abc123");
}

TEST(StoreTest, KeyExpiredAfterTTL) {
    Store store;
    store.setWithExpiry("session", "abc123", 1);
    std::this_thread::sleep_for(1500ms);
    auto result = store.get("session");
    EXPECT_FALSE(result.has_value());
}

TEST(StoreTest, ExistsReturnsFalseAfterExpiry) {
    Store store;
    store.setWithExpiry("session", "abc123", 1);
    std::this_thread::sleep_for(1500ms);
    EXPECT_FALSE(store.exists("session"));
}

TEST(StoreTest, SetOverwritesClearsExpiry) {
    Store store;
    store.setWithExpiry("key", "value", 1);
    store.set("key", "refreshed");                // overwrite without TTL
    std::this_thread::sleep_for(1500ms);
    auto result = store.get("key");               // should still exist
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "refreshed");
}