cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("kvstore")

add_executable (kvstore
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

add_executable (benchmark
    "benchmark.cpp"
)

set_property(TARGET benchmark PROPERTY CXX_STANDARD 20)
target_link_libraries(benchmark ws2_32)