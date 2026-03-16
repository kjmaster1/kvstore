# kvstore

A multi-threaded, in-memory key-value store written in C++20 — inspired by Redis. Accepts concurrent TCP client connections, supports TTL-based key expiry, and includes a benchmarking tool to measure throughput.

**~52,000 operations/sec** on a single machine (release build, synchronous client).

---

## Features

- `SET`, `GET`, `DEL`, `EXISTS`, `EXPIRE`, `PING` commands over TCP
- Thread-safe store using `std::mutex` with RAII `lock_guard` — no data races under concurrent load
- TTL expiry using `std::chrono::steady_clock` — keys auto-delete after a configurable number of seconds
- One thread per client connection via `std::thread`
- Graceful shutdown on `SIGINT`/`SIGTERM` — socket is closed cleanly, no orphaned connections
- Benchmarking tool measuring SET, GET, and DEL throughput in ops/sec
- Text protocol with Redis-inspired response prefixes (`+`, `-`, `:`, `$-1`)

---

## Tech Stack

| Technology | Purpose |
|---|---|
| C++20 | Core language |
| CMake | Build system |
| Winsock2 | TCP networking (Windows) |
| `std::thread` | Per-client concurrency |
| `std::mutex` / `lock_guard` | Thread-safe store access |
| `std::optional` | Nullable return values without exceptions |
| `std::chrono::steady_clock` | Monotonic TTL expiry |
| `std::atomic<bool>` | Lock-free shutdown flag |

---

## Architecture

```
kvstore/
├── main.cpp              # Entry point — creates and runs the server
├── Server.h / .cpp       # TCP socket setup, accept loop, signal handling
├── ClientHandler.h / .cpp # Per-client thread: command parsing and execution
├── Store.h / .cpp        # Thread-safe key-value store with TTL support
├── benchmark.cpp         # Standalone benchmarking tool
└── CMakeLists.txt        # Build configuration — two targets: kvstore, benchmark
```

### Request lifecycle

```
Client connects via TCP
    └── Server::run() calls accept()
        └── Spawns std::thread → handleClient()
            └── recv() reads bytes into accumulation buffer
                └── Lines split on \n, stripped of \r
                    └── parseCommand() tokenises the line
                        └── executeCommand() dispatches to Store
                            └── send() writes response back to client
```

### Thread safety

All public methods on `Store` acquire a `lock_guard<mutex>` before accessing `data` or `expiries`. `lock_guard` uses RAII — the mutex is released automatically when the guard goes out of scope, even if an exception is thrown. `Store::isExpired()` is an internal helper that must only be called while the lock is already held.

The server's shutdown flag is `std::atomic<bool>` rather than a plain `bool` — reads and writes are guaranteed to be indivisible across threads without needing a mutex.

---

## Protocol

A simplified subset of the Redis Serialisation Protocol (RESP). Commands are sent as plain text lines terminated with `\r\n`. Responses are prefixed to indicate type:

| Prefix | Meaning | Example |
|---|---|---|
| `+` | Success / string value | `+OK`, `+Kurt` |
| `-` | Error | `-ERR wrong number of arguments` |
| `:` | Integer | `:1`, `:0` |
| `$-1` | Nil (key not found) | `$-1` |

Commands are case-insensitive — `SET`, `set`, and `Set` are all valid.

### Supported commands

```
SET key value           Store a key-value pair
GET key                 Retrieve a value, or $-1 if not found
DEL key                 Delete a key — returns :1 if deleted, :0 if not found
EXISTS key              Returns :1 if key exists and has not expired, :0 otherwise
EXPIRE key seconds      Set a TTL on an existing key in seconds
PING                    Returns +PONG — useful for health checks
```

---

## Building

**Prerequisites:** CMake 3.10+, Visual Studio 2019/2022 with the Desktop development with C++ workload.

```cmd
git clone https://github.com/kjmaster1/kvstore
cd kvstore
cmake -S . -B out/build/x64-release -A x64
cmake --build out/build/x64-release --config Release
```

Executables are written to `out/build/x64-release/Release/`.

---

## Running

Start the server:

```cmd
.\kvstore.exe
```

```
kvstore listening on port 6380
Press Ctrl+C to stop
```

Connect with any TCP client — telnet, netcat, or a custom client:

```cmd
telnet localhost 6380
```

```
PING
+PONG
SET name Kurt
+OK
GET name
+Kurt
EXPIRE name 5
:1
GET name
+Kurt
# (6 seconds later)
GET name
$-1
```

---

## Benchmark

With the server running, execute the benchmark tool:

```cmd
.\benchmark.exe
```

Example output (release build, Windows, synchronous client):

```
kvstore benchmark
Operations per test: 10000
Connecting to 127.0.0.1:6380

SET:
  10000 operations in 0.193s
  51863 ops/sec

GET:
  10000 operations in 0.192s
  52190 ops/sec

DEL:
  10000 operations in 0.186s
  53859 ops/sec

Benchmark complete.
```

The benchmark uses a single synchronous client — one request in flight at a time. A pipelined client sending multiple commands before reading responses would achieve significantly higher throughput.

**Debug vs release:** the debug build achieves ~33,000 ops/sec. The release build achieves ~52,000 ops/sec — a 57% improvement from compiler optimisations alone (`/O2` inlining, dead code elimination, register allocation).

---

## Key Design Decisions

**RAII locking over manual lock/unlock** — `lock_guard` guarantees the mutex is always released, even if an exception propagates through a store method. Manual `mutex.unlock()` calls are easy to forget and leave the store deadlocked.

**`std::atomic<bool>` for the shutdown flag** — a plain `bool` modified from a signal handler and read from the main thread is undefined behaviour in C++. `atomic<bool>` makes the operation well-defined without the overhead of a mutex.

**`std::optional` return from `Store::get()`** — returning an empty `std::string` for a missing key is ambiguous — the key might genuinely be set to an empty string. `optional` makes the absent/present distinction explicit at the type level.

**Monotonic clock for TTL** — `steady_clock` is guaranteed never to go backwards, unlike the system clock which can be adjusted by NTP or the user. Using the system clock for TTL expiry could cause keys to never expire, or expire immediately, after a clock change.

**Detached threads over a thread pool** — for a portfolio project, detached threads keep the connection handling simple and readable. A production system would use a fixed thread pool to bound memory usage and prevent resource exhaustion under high connection load.
