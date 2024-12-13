#include <cpp_log/log.hpp>
#include <thread>
#include <chrono>

int main() {
    // 创建一个自定义的logger，使用自己的io_context
    auto ioc = std::make_shared<boost::asio::io_context>();
    cpp_log::Logger logger(ioc);
    
    // 添加异步控制台输出
    auto console_sink = std::make_shared<cpp_log::AsyncConsoleSink>(logger.get_io_context());
    console_sink->set_formatter(std::make_shared<cpp_log::DefaultFormatter>());
    logger.add_sink(console_sink);
    
    // 添加异步文件输出
    auto file_sink = std::make_shared<cpp_log::AsyncFileSink>(
        logger.get_io_context(), 
        "test.log"
    );
    file_sink->set_formatter(std::make_shared<cpp_log::DefaultFormatter>());
    logger.add_sink(file_sink);
    
    // 启动io_context运行线程
    std::thread io_thread([ioc]() {
        ioc->run();
    });
    
    // 测试不同级别的日志
    for (int i = 0; i < 5; ++i) {
        logger.debug(std::source_location::current(), "Debug message {}", i);
        logger.info(std::source_location::current(), "Info message {}", i);
        logger.warn(std::source_location::current(), "Warning message {}", i);
        logger.error(std::source_location::current(), "Error message {}", i);
        logger.fatal(std::source_location::current(), "Fatal message {}", i);
        
        // 模拟一些处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 使用全局日志函数（使用默认logger）
    CPP_LOG_INFO("Using global logger");
    CPP_LOG_DEBUG("Debug from global logger");
    
    // 等待所有日志写入完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 停止io_context
    ioc->stop();
    io_thread.join();
    
    return 0;
}
