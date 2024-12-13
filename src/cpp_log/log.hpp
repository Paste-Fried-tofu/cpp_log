#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <source_location>
#include <chrono>
#include <format>
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <memory>
#include <optional>

#include<boost/asio/io_context.hpp>
#include "cpp_log/level.hpp"
#include "cpp_log/sink.hpp"
#include "cpp_log/formatter.hpp"
#include "cpp_log/async_sink.hpp"

namespace cpp_log {

namespace asio = boost::asio;

// 日志记录器类
class Logger {
public:
    Logger(std::shared_ptr<asio::io_context> ioc = nullptr)
        : min_level_(Level::Debug)
        , io_context_(ioc ? ioc : std::make_shared<asio::io_context>()) {}
    
    ~Logger() = default;

    // 获取io_context
    asio::io_context& get_io_context() { return *io_context_; }

    // 添加输出目标，返回sink的索引
    size_t add_sink(std::shared_ptr<LogSink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.push_back(std::move(sink));
        return sinks_.size() - 1;
    }

    // 清除所有输出目标
    void clear_sinks() {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.clear();
    }

    // 根据索引获取输出对象
    std::shared_ptr<LogSink> get_sink(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index < sinks_.size()) {
            return sinks_[index];
        }
        return nullptr;
    }

    // 设置全局最小日志等级
    void set_level(Level level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }

    // 获取全局最小日志等级
    Level level() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return min_level_;
    }// 检查是否应该记录该等级的日志
    bool should_log(Level level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return level >= min_level_;
    }template<typename... Args>
    void log(Level level,
             const std::source_location& location,
             std::format_string<Args...> fmt,
             Args&&... args) {
        // 首先检查全局日志等级
        if (!should_log(level)) {
            return;
        }

        // 构建日志上下文
        LogContext context{
            .level = level,
            .timestamp = std::chrono::system_clock::now(),
            .location = location,
            .thread_id = std::this_thread::get_id(),
            .message = std::format(fmt, std::forward<Args>(args)...)
        };
        
        std::lock_guard<std::mutex> lock(mutex_);
        // 写入所有输出目标
        for (auto& sink : sinks_) {
            sink->write(context);
        }
    }

    template<typename... Args>
    void debug(const std::source_location& location,std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Debug, location, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const std::source_location& location,
             std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Info, location, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const std::source_location& location,std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Warning, location, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const std::source_location& location,std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Error, location, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(const std::source_location& location,
              std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Fatal, location, fmt, std::forward<Args>(args)...);
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    Level min_level_;// 全局最小日志等级
    std::shared_ptr<asio::io_context> io_context_;
};

// 默认的全局日志记录器
namespace detail {
    class DefaultLogger {
    public:
        static Logger& instance() {
            static DefaultLogger instance;
            return instance.logger_;
        }

    private:
        DefaultLogger() {
            // 创建异步控制台输出
            auto console_sink = std::make_shared<AsyncConsoleSink>(logger_.get_io_context());
            console_sink->set_formatter(std::make_shared<DefaultFormatter>());
            logger_.add_sink(console_sink);
            
            // 启动io_context运行线程
            auto& ioc = logger_.get_io_context();
            io_thread_ = std::thread([&ioc]() {
                ioc.run();
            });io_thread_.detach();  // 让线程在后台运行
        }

        Logger logger_;
        std::thread io_thread_;
    };

    inline Logger& default_logger() {
        return DefaultLogger::instance();
    }
} // namespace detail

// 全局便捷函数，使用默认的日志记录器
template<typename... Args>
void debug(const std::source_location& location,
          std::format_string<Args...> fmt, Args&&... args) {
    detail::default_logger().debug(location, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(const std::source_location& location,
         std::format_string<Args...> fmt, Args&&... args) {
    detail::default_logger().info(location, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(const std::source_location& location,
         std::format_string<Args...> fmt, Args&&... args) {
    detail::default_logger().warn(location, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void error(const std::source_location& location,std::format_string<Args...> fmt, Args&&... args) {
    detail::default_logger().error(location, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void fatal(const std::source_location& location,
          std::format_string<Args...> fmt, Args&&... args) {
    detail::default_logger().fatal(location, fmt, std::forward<Args>(args)...);
}

// 设置全局日志等级的便捷函数
inline void set_level(Level level) {
    detail::default_logger().set_level(level);
}

// 宏定义，简化使用（可选）
#define CPP_LOG_DEBUG(...) ::cpp_log::debug(std::source_location::current(), __VA_ARGS__)
#define CPP_LOG_INFO(...) ::cpp_log::info(std::source_location::current(), __VA_ARGS__)
#define CPP_LOG_WARN(...) ::cpp_log::warn(std::source_location::current(), __VA_ARGS__)
#define CPP_LOG_ERROR(...) ::cpp_log::error(std::source_location::current(), __VA_ARGS__)
#define CPP_LOG_FATAL(...) ::cpp_log::fatal(std::source_location::current(), __VA_ARGS__)

} // namespace cpp_log
