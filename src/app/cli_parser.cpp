#include "sonarlock/app/cli.hpp"

namespace sonarlock::app {

namespace {

core::Status parse_double(const std::string& value, double& out) {
    try {
        out = std::stod(value);
    } catch (...) {
        return core::Status::error(core::kErrInvalidArgument, "failed to parse number: " + value);
    }
    return core::Status::success();
}

core::Status parse_size(const std::string& value, std::size_t& out) {
    try {
        out = static_cast<std::size_t>(std::stoul(value));
    } catch (...) {
        return core::Status::error(core::kErrInvalidArgument, "failed to parse integer: " + value);
    }
    return core::Status::success();
}

} // namespace

core::Status parse_args(const std::vector<std::string>& args, CommandLine& out) {
    if (args.empty()) {
        out.kind = CommandKind::Help;
        return core::Status::success();
    }

    if (args[0] == "devices") {
        out.kind = CommandKind::Devices;
    } else if (args[0] == "run") {
        out.kind = CommandKind::Run;
    } else {
        return core::Status::error(core::kErrInvalidArgument, "unknown command: " + args[0]);
    }

    for (std::size_t i = 1; i < args.size(); ++i) {
        const auto& token = args[i];
        auto require_value = [&]() -> core::Status {
            if (i + 1 >= args.size()) {
                return core::Status::error(core::kErrInvalidArgument, "missing value for " + token);
            }
            ++i;
            return core::Status::success();
        };

        if (token == "--duration") {
            auto status = require_value(); if (!status.ok()) return status;
            if (!(status = parse_double(args[i], out.config.duration_seconds)).ok()) return status;
        } else if (token == "--freq") {
            auto status = require_value(); if (!status.ok()) return status;
            if (!(status = parse_double(args[i], out.config.tone_frequency_hz)).ok()) return status;
        } else if (token == "--samplerate") {
            auto status = require_value(); if (!status.ok()) return status;
            if (!(status = parse_double(args[i], out.config.sample_rate_hz)).ok()) return status;
        } else if (token == "--frames") {
            auto status = require_value(); if (!status.ok()) return status;
            if (!(status = parse_size(args[i], out.config.frames_per_buffer)).ok()) return status;
        } else if (token == "--backend") {
            auto status = require_value(); if (!status.ok()) return status;
            if (args[i] == "real") out.backend = core::BackendKind::Real;
            else if (args[i] == "fake") out.backend = core::BackendKind::Fake;
            else return core::Status::error(core::kErrInvalidArgument, "invalid backend: " + args[i]);
        } else if (token == "--fake-input") {
            auto status = require_value(); if (!status.ok()) return status;
            if (args[i] == "silence") out.fake_mode = core::FakeInputMode::Silence;
            else if (args[i] == "tone-noise") out.fake_mode = core::FakeInputMode::ToneWithNoise;
            else return core::Status::error(core::kErrInvalidArgument, "invalid fake input mode: " + args[i]);
        } else {
            return core::Status::error(core::kErrInvalidArgument, "unknown option: " + token);
        }
    }

    return core::Status::success();
}

} // namespace sonarlock::app
