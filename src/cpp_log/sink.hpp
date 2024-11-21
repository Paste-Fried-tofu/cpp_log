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

    // 设置日志等级
    void set_level(Level level) {
        level_ = level;
    }

    // 获取当前日志等级
    Level level() const {
        return level_;
    }

    // 检查是否应该记录此等级的日志
    bool should_log(Level msg_level) const {
        return static_cast<int>(msg_level) >= static_cast<int>(level_);
    }

    // 设置格式化器
    void set_formatter(std::shared_ptr<LogFormatter> formatter) {
        formatter_ = formatter;
    }

    // 写入日志（纯虚函数）
    virtual void write(const LogContext& context) = 0;

    // 刷新输出缓冲
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
        auto color = get_color(context.level);
        
        std::cout << color.code;
        std::cout << formatted;
        std::cout << ColorCode::reset;
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
        file_ << formatted;
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
    // 基于大小的轮转构造函数
    RotatingFileSink(const std::string& filename,
                     size_t max_size = 10 * 1024 * 1024,  // 默认10MB
                     size_t max_files = 5)                 // 默认保留5个文件
        : FileSink(filename)
        , base_filename_(filename)
        , max_size_(max_size)
        , max_files_(max_files)
        , strategy_(RotationStrategy::Size)
        , next_rotation_time_(std::chrono::system_clock::now()) {
        current_size_ = std::filesystem::file_size(filename);
    }

    // 基于时间的轮转构造函数
    RotatingFileSink(const std::string& filename,
                     RotationStrategy strategy,
                     size_t max_files = 5)
        : FileSink(filename)
        , base_filename_(filename)
        , max_size_(0)  // 不使用大小限制
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
        size_t msg_size = formatted.size();

        // 检查是否需要轮转
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

        file_ << formatted;
        current_size_ += msg_size;
    }

private:
    void calculate_next_rotation_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);

        // 重置分钟和秒
        tm_now.tm_min = 0;
        tm_now.tm_sec = 0;

        if (strategy_ == RotationStrategy::Daily) {
            // 设置为明天的00:00
            tm_now.tm_hour = 0;
            tm_now.tm_mday += 1;
        } else if (strategy_ == RotationStrategy::Hourly) {
            // 设置为下一个小时的00:00
            tm_now.tm_hour += 1;
        }

        next_rotation_time_ = std::chrono::system_clock::from_time_t(std::mktime(&tm_now));
    }

    void rotate_files() {
        file_.close();  // 关闭当前文件

        // 生成轮转后的文件名
        std::string rotated_name = generate_rotated_filename();

        // 重命名当前文件
        if (std::filesystem::exists(base_filename_)) {
            std::filesystem::rename(base_filename_, rotated_name);
        }

        // 删除最旧的文件（如果超过最大文件数）
        cleanup_old_files();

        // 重新打开一个新文件
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
            // 使用数字后缀
            oss << "1";
        } else {
            // 使用时间戳后缀
            oss << std::put_time(&tm_now, "%Y%m%d-%H%M%S");
        }

        return oss.str();
    }

    void cleanup_old_files() {
        std::vector<std::string> log_files;
        auto base_path = std::filesystem::path(base_filename_).parent_path();
        auto base_stem = std::filesystem::path(base_filename_).stem().string();

        // 收集所有相关的日志文件
        for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(base_stem) == 0 && filename != std::filesystem::path(base_filename_).filename()) {
                    log_files.push_back(entry.path().string());
                }
            }
        }

        // 按修改时间排序
        std::sort(log_files.begin(), log_files.end(), 
            [](const std::string& a, const std::string& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });

        // 删除超出限制的旧文件
        while (log_files.size() >= max_files_) {
            std::filesystem::remove(log_files.back());
            log_files.pop_back();
        }
    }

    std::string base_filename_;     // 基础文件名
    size_t max_size_;              // 单个文件最大大小（字节）
    size_t max_files_;             // 最大保留文件数
    size_t current_size_;          // 当前文件大小
    RotationStrategy strategy_;     // 轮转策略
    std::chrono::system_clock::time_point next_rotation_time_;  // 下次轮转时间
};

} // namespace cpp_log
