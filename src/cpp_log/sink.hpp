#pragma once

#include <memory>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "cpp_log/level.hpp"
#include "cpp_log/color.hpp"
#include "cpp_log/formatter.hpp"

namespace cpp_log {

// 日志输出基类
class LogSink {
public:
    virtual ~LogSink() = default;

    void set_level(Level level) {
        level_ = level;
    }

    Level level() const {
        return level_;
    }

    bool should_log(Level msg_level) const {
        return static_cast<int>(msg_level) >= static_cast<int>(level_);
    }

    void set_formatter(std::shared_ptr<LogFormatter> formatter) {
        formatter_ = formatter;
    }

    virtual void write(const LogContext& context) = 0;

    virtual void flush() {
        // 默认实现为空
    }

protected:
    Level level_ = Level::Debug;  // 默认记录所有日志
    std::shared_ptr<LogFormatter> formatter_;
};

// 控制台输出
class ConsoleSink : public LogSink {
public:
    ConsoleSink() {
        formatter_ = std::make_shared<DefaultFormatter>();
    }

    void write(const LogContext& context) override {
        if (!should_log(context.level)) {
            return;
        }

        std::string formatted = formatter_->format(context);
        std::cout << formatted;
    }

    void flush() override {
        std::cout.flush();
    }
};

// 文件输出
class FileSink : public LogSink {
public:
    FileSink(const std::string& filename) : file_(filename, std::ios::app) {
        formatter_ = std::make_shared<DefaultFormatter>();
    }

    void write(const LogContext& context) override {
        if (!should_log(context.level)) {
            return;
        }

        std::string formatted = formatter_->format(context);
        // 移除颜色代码后写入文件
        file_ << color::strip_color_codes(formatted);
    }

    void flush() override {
        file_.flush();
    }

protected:  
    std::ofstream file_;
};

// 日志轮转策略
enum class RotationStrategy {
    Size,    // 基于文件大小
    Daily,   // 每天轮转
    Hourly   // 每小时轮转
};

// 支持文件轮转的文件输出
class RotatingFileSink : public FileSink {
public:
    RotatingFileSink(const std::string& filename,
                     size_t max_size = 10 * 1024 * 1024,
                     size_t max_files = 5)
        : FileSink(filename)
        , base_filename_(filename)
        , max_size_(max_size)
        , max_files_(max_files)
        , strategy_(RotationStrategy::Size)
        , next_rotation_time_(std::chrono::system_clock::now()) {
        current_size_ = std::filesystem::file_size(filename);
    }

    RotatingFileSink(const std::string& filename,
                     RotationStrategy strategy,
                     size_t max_files = 5)
        : FileSink(filename)
        , base_filename_(filename)
        , max_size_(0)
        , max_files_(max_files)
        , strategy_(strategy) {
        current_size_ = std::filesystem::file_size(filename);
        calculate_next_rotation_time();
    }

    void write(const LogContext& context) override {
        if (!should_log(context.level)) {
            return;
        }

        std::string formatted = formatter_->format(context);
        // 移除颜色代码后计算消息大小
        std::string stripped = color::strip_color_codes(formatted);
        size_t msg_size = stripped.size();

        bool should_rotate = false;
        auto now = std::chrono::system_clock::now();

        switch (strategy_) {
            case RotationStrategy::Size:
                should_rotate = (current_size_ + msg_size > max_size_);
                break;
            case RotationStrategy::Daily:
            case RotationStrategy::Hourly:
                should_rotate = (now >= next_rotation_time_);
                break;
        }

        if (should_rotate) {
            rotate_files();
            current_size_ = 0;
            if (strategy_ != RotationStrategy::Size) {
                calculate_next_rotation_time();
            }
        }

        file_ << stripped;
        current_size_ += msg_size;
    }

private:
    void calculate_next_rotation_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);

        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;

        if (strategy_ == RotationStrategy::Daily) {
            tm_now.tm_hour = 0;
            tm_now.tm_mday += 1;
        } else if (strategy_ == RotationStrategy::Hourly) {
            tm_now.tm_hour += 1;
        }

        next_rotation_time_ = std::chrono::system_clock::from_time_t(std::mktime(&tm_now));
    }

    void rotate_files() {
        file_.close();

        std::string rotated_name = generate_rotated_filename();

        if (std::filesystem::exists(base_filename_)) {
            std::filesystem::rename(base_filename_, rotated_name);
        }

        cleanup_old_files();

        file_.open(base_filename_, std::ios::out | std::ios::trunc);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open new log file after rotation: " + base_filename_);
        }
    }

    std::string generate_rotated_filename() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);

        std::ostringstream oss;
        oss << base_filename_ << ".";

        if (strategy_ == RotationStrategy::Size) {
            oss << "1";
        } else {
            oss << std::put_time(&tm_now, "%Y%m%d-%H%M%S");
        }

        return oss.str();
    }

    void cleanup_old_files() {
        std::vector<std::string> log_files;
        auto base_path = std::filesystem::path(base_filename_).parent_path();
        auto base_stem = std::filesystem::path(base_filename_).stem().string();

        for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(base_stem) == 0 && filename != std::filesystem::path(base_filename_).filename()) {
                    log_files.push_back(entry.path().string());
                }
            }
        }

        std::sort(log_files.begin(), log_files.end(),
            [](const std::string& a, const std::string& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });

        while (log_files.size() >= max_files_) {
            std::filesystem::remove(log_files.back());
            log_files.pop_back();
        }
    }

    std::string base_filename_;
    size_t max_size_;
    size_t max_files_;
    size_t current_size_;
    RotationStrategy strategy_;
    std::chrono::system_clock::time_point next_rotation_time_;
};

} // namespace cpp_log
