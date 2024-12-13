#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <queue>
#include <memory>
#include "cpp_log/sink.hpp"
#include "cpp_log/color.hpp"

namespace cpp_log {

namespace asio = boost::asio;

//异步日志sink基类
class AsyncLogSink : public LogSink {
public:
    explicit AsyncLogSink(asio::io_context& ioc)
        : strand_(asio::make_strand(ioc)), running_(true) {
        // 启动异步处理循环
        asio::co_spawn(strand_, process_loop(), asio::detached);
    }

    ~AsyncLogSink() {
        running_ = false;
    }

    // 重写write方法，将日志消息和级别加入队列
    void write(const LogContext& context) override {
        if (!should_log(context.level)) {
            return;
        }

        std::string formatted = formatter_->format(context);
        asio::post(strand_, [this, formatted = std::move(formatted), level = context.level]() {
            message_queue_.push({std::move(formatted), level});
        });
    }

protected:
    // 实际的写入操作，由派生类实现
    virtual asio::awaitable<void> do_write(const std::string& message, Level level) = 0;

private:
    // 异步处理循环
    asio::awaitable<void> process_loop() {
        while (running_) {
            if (!message_queue_.empty()) {
                auto [message, level] = std::move(message_queue_.front());
                message_queue_.pop();
                co_await do_write(message, level);
            }
            co_await asio::post(strand_, asio::use_awaitable);
        }
    }

    struct QueueEntry {
        std::string message;
        Level level;
    };

    asio::strand<asio::io_context::executor_type> strand_;
    std::queue<QueueEntry> message_queue_;
    std::atomic<bool> running_;
};

// 异步控制台输出
class AsyncConsoleSink : public AsyncLogSink {
public:
    explicit AsyncConsoleSink(asio::io_context& ioc)
        : AsyncLogSink(ioc) {}

protected:
    asio::awaitable<void> do_write(const std::string& message, Level level) override {
        auto color = get_color(level);
        std::cout << color.code << message << color.reset;std::cout.flush();
        co_return;
    }
};

// 异步文件输出
class AsyncFileSink : public AsyncLogSink {
public:
    AsyncFileSink(asio::io_context& ioc, const std::string& filename)
        : AsyncLogSink(ioc)
        , file_(filename, std::ios::app) {}

protected:
    asio::awaitable<void> do_write(const std::string& message, Level level) override {
        file_ << message;
        file_.flush();
        co_return;
    }

private:
    std::ofstream file_;
};

} // namespace cpp_log
