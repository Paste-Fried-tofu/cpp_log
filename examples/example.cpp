#include <thread>
#include <chrono>
#include <filesystem>
#include "cpp_log/log.hpp"

// 工作线程函数
void worker(int id, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        CPP_LOG_DEBUG("Worker {} iteration {}", id, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 生成大量日志的函数
void generate_logs(cpp_log::Logger& logger, int count) {
    for (int i = 0; i < count; ++i) {
        std::string large_message(1024, 'X');  // 1KB的消息
        logger.info(std::source_location::current(), 
                   "Log message #{} with large content: {}", i, large_message);
    }
}

int main() {
    // 确保日志目录存在
    std::filesystem::create_directories("logs");

    // 创建自定义的日志记录器
    cpp_log::Logger logger;

    // 添加控制台输出，设置为INFO级别
    auto console_sink = std::make_shared<cpp_log::ConsoleSink>();
    console_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "%t [%l] Thread-%d > %m"
    ));
    console_sink->set_level(cpp_log::Level::Info);  // 控制台只显示INFO及以上级别
    logger.add_sink(console_sink);

    // 添加基于大小轮转的文件输出
    auto size_rotating_sink = std::make_shared<cpp_log::RotatingFileSink>(
        "logs/size_example.log",    // 基础文件名
        100 * 1024,                // 100KB
        3                          // 保留3个备份文件
    );
    size_rotating_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "[%l] %t - %m (File: %f:%n)"
    ));
    size_rotating_sink->set_level(cpp_log::Level::Debug);
    logger.add_sink(size_rotating_sink);

    // 添加每小时轮转的文件输出
    auto hourly_rotating_sink = std::make_shared<cpp_log::RotatingFileSink>(
        "logs/hourly_example.log",           // 基础文件名
        cpp_log::RotationStrategy::Hourly,
        24                                   // 保留24个文件（一天的日志）
    );
    hourly_rotating_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "[%l] %t - %m (File: %f:%n)"
    ));
    hourly_rotating_sink->set_level(cpp_log::Level::Debug);
    logger.add_sink(hourly_rotating_sink);

    // 添加每天轮转的文件输出
    auto daily_rotating_sink = std::make_shared<cpp_log::RotatingFileSink>(
        "logs/daily_example.log",            // 基础文件名
        cpp_log::RotationStrategy::Daily,
        7                                    // 保留7天的日志
    );
    daily_rotating_sink->set_formatter(std::make_shared<cpp_log::PatternFormatter>(
        "[%l] %t - %m (File: %f:%n)"
    ));
    daily_rotating_sink->set_level(cpp_log::Level::Debug);
    logger.add_sink(daily_rotating_sink);

    // 使用自定义的日志记录器
    logger.info(std::source_location::current(), "Starting the application");
    logger.debug(std::source_location::current(), "Debug message with value: {}", 42);
    logger.warn(std::source_location::current(), "Warning: resource usage at {}%", 80);
    logger.error(std::source_location::current(), "Failed to process item {}", "data.txt");
    logger.fatal(std::source_location::current(), "Critical error in module {}", "core");

    // 创建工作线程
    std::thread workers[3];
    for (int i = 0; i < 3; ++i) {
        workers[i] = std::thread(worker, i, 3);
    }

    // 等待所有工作线程完成
    for (auto& worker : workers) {
        worker.join();
    }

    // 生成大量日志来测试文件轮转
    logger.info(std::source_location::current(), "Starting log rotation test...");
    generate_logs(logger, 500);  // 生成500条日志消息
    logger.info(std::source_location::current(), "Log rotation test completed");

    return 0;
}
