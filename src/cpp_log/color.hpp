#pragma once
#include <string>
#include <regex>
#include "cpp_log/level.hpp"

namespace cpp_log {
namespace color {

// 颜色代码
constexpr const char* reset   = "\033[0m";
constexpr const char* black   = "\033[30m";
constexpr const char* red     = "\033[31m";
constexpr const char* green   = "\033[32m";
constexpr const char* yellow  = "\033[33m";
constexpr const char* blue    = "\033[34m";
constexpr const char* magenta = "\033[35m";
constexpr const char* cyan    = "\033[36m";
constexpr const char* white   = "\033[37m";
constexpr const char* bold_red = "\033[1;31m";

// 移除ANSI颜色代码
inline std::string strip_color_codes(const std::string& str) {
    static const std::regex color_regex("\033\\[[0-9;]*m");
    return std::regex_replace(str, color_regex, "");
}

} // namespace color

// 获取日志级别对应的颜色
inline const char* get_level_color(Level level) {
    switch (level) {
        case Level::Debug:
            return color::green;
        case Level::Info:
            return color::reset;
        case Level::Warning:
            return color::yellow;
        case Level::Error:
            return color::red;
        case Level::Fatal:
            return color::bold_red;
        default:
            return color::reset;
    }
}

} // namespace cpp_log
