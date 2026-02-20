#pragma once

#include <string_view>

namespace sonarlock::core {

enum class LogLevel {
    Info,
    Warn,
    Error,
};

void log(LogLevel level, std::string_view message);

} // namespace sonarlock::core
