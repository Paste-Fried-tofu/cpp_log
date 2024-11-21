#pragma once

#include <string_view>
#include "cpp_log/level.hpp"

namespace cpp_log {

struct ColorCode {
    std::string_view code;
    static constexpr std::string_view reset = "\033[0m";
};

constexpr ColorCode get_color(Level level) {
    switch (level) {
        case Level::Debug:   return {"\033[32m"}; // Green
        case Level::Info:    return {"\033[0m"};  // Default
        case Level::Warning: return {"\033[33m"}; // Yellow
        case Level::Error:   return {"\033[31m"}; // Red
        case Level::Fatal:   return {"\033[1;31m"}; // Bold Red
        default:            return {"\033[0m"};
    }
}

} // namespace cpp_log
