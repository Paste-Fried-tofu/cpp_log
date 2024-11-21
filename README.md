# cpp_log

A modern, header-only C++ logging library with flexible log rotation support.

> **Note**: This project was primarily developed with the AI(By WindSurf IDE), so there may be existing large bugs or issues. Please use it with caution. After all, this is a toy project.


## Features

- 🚀 Modern C++20 implementation
- 📝 Header-only library
- 🔄 Multiple log rotation strategies:
  - Size-based rotation (e.g., rotate when file reaches 100MB)
  - Time-based rotation (hourly or daily)
  - Configurable number of backup files
- 🎯 Multiple output sinks:
  - Console output
  - File output with rotation support
- 🎨 Customizable log formatting
- 🔧 Thread-safe logging
- 📦 CMake and pkg-config integration
- 🧵 Asynchronous logging support

## Requirements

- C++20 compliant compiler
- CMake 3.15 or higher

## Installation

### From Source

```bash
git clone https://github.com/Paste-Fried-tofu/cpp_log.git
cd cpp_log
mkdir build && cd build
cmake ..
make install
```

### CMake Integration

In your project's CMakeLists.txt:

```cmake
find_package(cpp_log 1.0 REQUIRED)
target_link_libraries(your_target PRIVATE cpp_log::cpp_log)
```

### pkg-config Integration

```bash
pkg-config --cflags cpp_log
```

## Usage

### Basic Usage

```cpp
#include <cpp_log/log.hpp>
#include <filesystem>

int main() {
    // Create logger
    cpp_log::Logger logger;

    // Add console sink
    auto console_sink = std::make_shared<cpp_log::ConsoleSink>();
    console_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "%t [%l] > %m"  // timestamp [level] > message
    ));
    logger.add_sink(console_sink);

    // Log messages
    logger.info(std::source_location::current(), "Hello, {}!", "World");
    logger.debug(std::source_location::current(), "Debug message");
    logger.error(std::source_location::current(), "Error: {}", "file not found");

    // Or use the convenience macros
    CPP_LOG_INFO("Hello, {}!", "World");
    CPP_LOG_DEBUG("Debug message");
    CPP_LOG_ERROR("Error: {}", "file not found");
}
```

### File Output with Rotation

```cpp
#include <cpp_log/log.hpp>
#include <filesystem>

int main() {
    // Ensure log directory exists
    std::filesystem::create_directories("logs");

    cpp_log::Logger logger;

    // Size-based rotation (rotate at 100KB, keep 3 backup files)
    auto size_rotating_sink = std::make_shared<cpp_log::RotatingFileSink>(
        "logs/app.log",    // base filename
        100 * 1024,        // rotate at 100KB
        3                  // keep 3 backup files
    );
    size_rotating_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "[%l] %t - %m"     // [level] timestamp - message
    ));
    logger.add_sink(size_rotating_sink);

    // Daily rotation (keep 7 days of logs)
    auto daily_rotating_sink = std::make_shared<cpp_log::RotatingFileSink>(
        "logs/daily.log",
        cpp_log::RotationStrategy::Daily,
        7  // keep 7 files
    );
    daily_rotating_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "[%l] %t - %m"
    ));
    logger.add_sink(daily_rotating_sink);

    // Log messages
    logger.info(std::source_location::current(), "Application started");
}
```

### Custom Formatting

```cpp
#include <cpp_log/log.hpp>

int main() {
    cpp_log::Logger logger;
    
    auto console_sink = std::make_shared<cpp_log::ConsoleSink>();
    
    // Create custom formatter
    auto formatter = std::make_shared<cpp_log::PatternFormatter>(
        "[%t] <%l> (%f:%n) %m"  // [timestamp] <level> (file:line) message
    );
    console_sink->set_formatter(formatter);
    console_sink->set_level(cpp_log::Level::Debug);  // Set minimum log level

    logger.add_sink(console_sink);
    
    logger.info(std::source_location::current(), "Custom formatted message");
    // Or use the macro
    CPP_LOG_INFO("Custom formatted message");
}
```

## Format Specifiers

The pattern formatter supports the following specifiers:
- `%t` - Timestamp
- `%l` - Log level
- `%f` - Source file
- `%n` - Line number
- `%d` - Thread ID
- `%m` - Log message
- `%%` - Literal %

## Log Rotation Details

### Size-based Rotation
- Rotates when file size exceeds the specified limit
- Keeps specified number of backup files
- Backup files are named with numerical suffixes (e.g., app.log.1, app.log.2)

### Time-based Rotation
- Daily: Rotates at midnight (00:00:00)
- Hourly: Rotates at the start of each hour
- Backup files include timestamps (e.g., app.log.20231225-235959)

## Thread Safety

The library is thread-safe by design:
- All logging operations are protected by mutexes
- Each sink handles its own synchronization
- Safe to use from multiple threads simultaneously

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
