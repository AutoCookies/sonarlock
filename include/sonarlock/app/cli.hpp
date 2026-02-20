#pragma once

#include "sonarlock/core/types.hpp"

#include <string>
#include <vector>

namespace sonarlock::app {

enum class CommandKind {
    Devices,
    Run,
    Help,
};

struct CommandLine {
    CommandKind kind{CommandKind::Help};
    core::AudioConfig config{};
    core::BackendKind backend{core::BackendKind::Fake};
    core::FakeInputMode fake_mode{core::FakeInputMode::Silence};
};

core::Status parse_args(const std::vector<std::string>& args, CommandLine& out);

} // namespace sonarlock::app
