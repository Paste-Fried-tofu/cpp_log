#pragma once

namespace cpp_log {

enum class Level {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

constexpr const char* get_level_string(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        case Level::Fatal:   return "FATAL";
        default:            return "UNKNOWN";
    }
}

} // namespace cpp_log