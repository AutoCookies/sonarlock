#pragma once

#include "sonarlock/core/types.hpp"

#include <memory>
#include <string_view>

namespace sonarlock::core {

enum class LogLevel { Info, Warn, Error };

class Logger {
  public:
    virtual ~Logger() = default;
    virtual void write(LogLevel level, std::string_view message) = 0;
};

std::unique_ptr<Logger> make_console_file_logger(const LoggingSection& cfg);

void log(LogLevel level, std::string_view message);
void set_global_logger(std::unique_ptr<Logger> logger);

} // namespace sonarlock::core
