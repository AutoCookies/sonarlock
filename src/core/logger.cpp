#include "sonarlock/core/logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace sonarlock::core {

namespace {
class ConsoleFileLogger final : public Logger {
  public:
    explicit ConsoleFileLogger(LoggingSection cfg) : cfg_(std::move(cfg)) {}

    void write(LogLevel level, std::string_view message) override {
        std::lock_guard<std::mutex> lock(mu_);
        rotate_if_needed();

        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ts;
        ts << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");

        const char* p = level == LogLevel::Warn ? "WARN" : (level == LogLevel::Error ? "ERROR" : "INFO");
        const std::string line = ts.str() + " [" + p + "] " + std::string(message) + "\n";
        std::cout << line;
        std::ofstream out(cfg_.file_path, std::ios::app);
        out << line;
    }

  private:
    void rotate_if_needed() {
        namespace fs = std::filesystem;
        if (!fs::exists(cfg_.file_path)) return;
        if (fs::file_size(cfg_.file_path) < cfg_.rotate_size_bytes) return;

        for (int i = static_cast<int>(cfg_.rotate_count) - 1; i >= 1; --i) {
            const std::string from = cfg_.file_path + "." + std::to_string(i);
            const std::string to = cfg_.file_path + "." + std::to_string(i + 1);
            if (fs::exists(from)) fs::rename(from, to);
        }
        fs::rename(cfg_.file_path, cfg_.file_path + ".1");
    }

    LoggingSection cfg_;
    std::mutex mu_;
};

std::unique_ptr<Logger> g_logger;
} // namespace

std::unique_ptr<Logger> make_console_file_logger(const LoggingSection& cfg) {
    return std::make_unique<ConsoleFileLogger>(cfg);
}

void set_global_logger(std::unique_ptr<Logger> logger) { g_logger = std::move(logger); }

void log(LogLevel level, std::string_view message) {
    if (!g_logger) g_logger = make_console_file_logger(LoggingSection{});
    g_logger->write(level, message);
}

} // namespace sonarlock::core
