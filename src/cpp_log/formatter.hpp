#pragma once

#include <string>
#include <chrono>
#include <format>
#include <sstream>
#include "cpp_log/level.hpp"
#include "cpp_log/color.hpp"

// 为 std::thread::id 添加格式化支持
template<>
struct std::formatter<std::thread::id> : std::formatter<std::string> {
    auto format(const std::thread::id& id, format_context& ctx) const {
        std::stringstream ss;
        ss << id;
        return formatter<string>::format(ss.str(), ctx);
    }
};

namespace cpp_log {

// 日志记录的上下文信息
struct LogContext {
    Level level;
    std::chrono::system_clock::time_point timestamp;
    std::source_location location;
    std::thread::id thread_id;
    std::string message;
};

// 日志格式化器接口
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogContext& context) = 0;
};

// 默认格式化器
class DefaultFormatter : public LogFormatter {
public:
    std::string format(const LogContext& context) override {
        auto time_str = std::format("{:%Y-%m-%d %H:%M:%S}", context.timestamp);
        auto level_color = get_level_color(context.level);
        
        return std::format("{}{}{} {}[{}]{} {}{}<{}:{}>{}{}(Thread {}){}{}{}{}",
            color::cyan, time_str, color::reset,
            level_color, get_level_string(context.level), color::reset,
            color::blue, context.location.file_name(), context.location.line(), color::reset,
            color::magenta, context.thread_id, color::reset,
            level_color, context.message, color::reset,
            "\n");
    }
};

// 自定义格式化器
class PatternFormatter : public LogFormatter {
public:
    // 格式模式说明：
    // %t - 时间戳
    // %l - 日志级别
    // %f - 文件名
    // %n - 行号
    // %d - 线程ID
    // %m - 日志消息
    // %% - % 字符
    explicit PatternFormatter(std::string pattern) : pattern_(std::move(pattern)) {}

    std::string format(const LogContext& context) override {
        std::string result = pattern_;
        auto time_str = std::format("{:%Y-%m-%d %H:%M:%S}", context.timestamp);

        // 替换所有的占位符
        size_t pos = 0;
        while ((pos = result.find('%', pos)) != std::string::npos) {
            if (pos + 1 >= result.length()) break;

            char specifier = result[pos + 1];
            std::string replacement;

            switch (specifier) {
                case 't': // 时间戳
                    replacement = time_str;
                    break;
                case 'l': // 日志级别
                    replacement = std::string(get_level_string(context.level));
                    break;
                case 'f': // 文件名
                    replacement = context.location.file_name();
                    break;
                case 'n': // 行号
                    replacement = std::to_string(context.location.line());
                    break;
                case 'd': // 线程ID
                    {
                        std::stringstream ss;
                        ss << context.thread_id;
                        replacement = ss.str();
                    }
                    break;
                case 'm': // 消息
                    replacement = context.message;
                    break;
                case '%': // 转义 %
                    replacement = "%";
                    break;
                default:
                    pos++;
                    continue;
            }

            result.replace(pos, 2, replacement);
            pos += replacement.length();
        }

        result += '\n';
        return result;
    }

private:
    std::string pattern_;
};

} // namespace cpp_log
