#include "sonarlock/core/logger.hpp"

#include <iostream>

namespace sonarlock::core {

void log(LogLevel level, std::string_view message) {
    const char* prefix = "[INFO]";
    if (level == LogLevel::Warn) {
        prefix = "[WARN]";
    } else if (level == LogLevel::Error) {
        prefix = "[ERROR]";
    }
    std::cout << prefix << ' ' << message << '\n';
}

} // namespace sonarlock::core
