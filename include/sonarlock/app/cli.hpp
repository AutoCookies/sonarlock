#pragma once

#include "sonarlock/core/types.hpp"

#include <string>
#include <vector>

namespace sonarlock::app {

enum class CommandKind { Devices, Run, Analyze, Calibrate, DumpEvents, Help };

struct CommandLine {
    CommandKind kind{CommandKind::Help};
    core::AudioConfig config{};
    core::BackendKind backend{core::BackendKind::Fake};
    std::string csv_path;
    std::string config_path;
    bool json_output{false};
    std::size_t dump_count{50};
};

core::Status parse_args(const std::vector<std::string>& args, CommandLine& out);
core::Status load_config_file(const std::string& path, core::AudioConfig& cfg);

} // namespace sonarlock::app
