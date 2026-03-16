#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "Store.h"

// Verifies that concurrent writes from multiple threads do not corrupt the store
TEST(ConcurrencyTest, ConcurrentWritesDoNotCorruptStore) {
    Store store;
    const int threadCount = 10;
    const int writesPerThread = 1000;
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; t++) {
        threads.emplace_back([&store, t, writesPerThread]() {
            for (int i = 0; i < writesPerThread; i++) {
                std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                store.set(key, std::to_string(i));
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all keys were written correctly
    int found = 0;
    for (int t = 0; t < threadCount; t++) {
        for (int i = 0; i < writesPerThread; i++) {
            std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
            auto result = store.get(key);
            if (result.has_value() && result.value() == std::to_string(i)) {
                found++;
            }
        }
    }

    EXPECT_EQ(found, threadCount * writesPerThread);
}

// Verifies that concurrent reads and writes do not cause crashes or data races
TEST(ConcurrencyTest, ConcurrentReadsAndWritesAreStable) {
    Store store;
    store.set("shared", "initial");

    std::atomic<bool> stop = false;
    std::vector<std::thread> threads;

    // Writer thread — continuously updates the shared key
    threads.emplace_back([&store, &stop]() {
        int i = 0;
        while (!stop) {
            store.set("shared", std::to_string(i++));
        }
        });

    // Reader threads — continuously read the shared key
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&store, &stop]() {
            while (!stop) {
                store.get("shared");
            }
            });
    }

    // Let threads run for 500ms then stop
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    // If we reach here without a crash or sanitizer error, the test passes
    SUCCEED();
}

// Verifies that concurrent deletes are handled correctly
TEST(ConcurrencyTest, ConcurrentDeletesDoNotCrash) {
    Store store;
    const int keyCount = 1000;

    for (int i = 0; i < keyCount; i++) {
        store.set("key_" + std::to_string(i), "value");
    }

    std::vector<std::thread> threads;

    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&store, keyCount]() {
            for (int i = 0; i < keyCount; i++) {
                store.del("key_" + std::to_string(i));
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    SUCCEED();
}