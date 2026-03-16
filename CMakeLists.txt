cmake_minimum_required (VERSION 3.14)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("kvstore")

# Download and build Google Test automatically
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent Google Test from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Main server executable
add_executable(kvstore
    "main.cpp"
    "Store.h"
    "Store.cpp"
    "ClientHandler.h"
    "ClientHandler.cpp"
    "Server.h"
    "Server.cpp"
)
set_property(TARGET kvstore PROPERTY CXX_STANDARD 20)
target_link_libraries(kvstore ws2_32)

# Benchmark executable
add_executable(benchmark
    "benchmark.cpp"
)
set_property(TARGET benchmark PROPERTY CXX_STANDARD 20)
target_link_libraries(benchmark ws2_32)

# Test executable
add_executable(kvstore_tests
    "tests/StoreTest.cpp"
    "tests/CommandParserTest.cpp"
    "tests/ConcurrencyTest.cpp"
    "Store.h"
    "Store.cpp"
    "ClientHandler.h"
    "ClientHandler.cpp"
)
set_property(TARGET kvstore_tests PROPERTY CXX_STANDARD 20)
target_link_libraries(kvstore_tests GTest::gtest_main ws2_32)
target_include_directories(kvstore_tests PRIVATE ${CMAKE_SOURCE_DIR})

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(kvstore_tests)