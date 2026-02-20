#include "sonarlock/app/cli.hpp"

namespace sonarlock::app {

namespace {

core::Status parse_double(const std::string& value, double& out) {
    try { out = std::stod(value); } catch (...) { return core::Status::error(core::kErrInvalidArgument, "bad number: " + value); }
    return core::Status::success();
}

core::Status parse_uint(const std::string& value, std::uint32_t& out) {
    try { out = static_cast<std::uint32_t>(std::stoul(value)); } catch (...) { return core::Status::error(core::kErrInvalidArgument, "bad integer: " + value); }
    return core::Status::success();
}

core::Status parse_size(const std::string& value, std::size_t& out) {
    try { out = static_cast<std::size_t>(std::stoul(value)); } catch (...) { return core::Status::error(core::kErrInvalidArgument, "bad integer: " + value); }
    return core::Status::success();
}

} // namespace

core::Status parse_args(const std::vector<std::string>& args, CommandLine& out) {
    if (args.empty()) return core::Status::success();

    if (args[0] == "devices") out.kind = CommandKind::Devices;
    else if (args[0] == "run") out.kind = CommandKind::Run;
    else if (args[0] == "analyze") out.kind = CommandKind::Analyze;
    else return core::Status::error(core::kErrInvalidArgument, "unknown command: " + args[0]);

    for (std::size_t i = 1; i < args.size(); ++i) {
        const auto& t = args[i];
        auto take = [&]() -> core::Status { if (i + 1 >= args.size()) return core::Status::error(core::kErrInvalidArgument, "missing value for " + t); ++i; return core::Status::success(); };
        core::Status st;

        if (t == "--duration") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.duration_seconds)).ok()) return st; }
        else if (t == "--freq" || t == "--f0") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.f0_hz)).ok()) return st; }
        else if (t == "--samplerate") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.sample_rate_hz)).ok()) return st; }
        else if (t == "--frames") { if (!(st = take()).ok() || !(st = parse_size(args[i], out.config.dsp.frames_per_buffer)).ok()) return st; }
        else if (t == "--lp-cutoff") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.lp_cutoff_hz)).ok()) return st; }
        else if (t == "--band-low") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.doppler_band_low_hz)).ok()) return st; }
        else if (t == "--band-high") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.dsp.doppler_band_high_hz)).ok()) return st; }
        else if (t == "--trigger-th") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.detection.trigger_threshold)).ok()) return st; }
        else if (t == "--release-th") { if (!(st = take()).ok() || !(st = parse_double(args[i], out.config.detection.release_threshold)).ok()) return st; }
        else if (t == "--debounce-ms") { if (!(st = take()).ok() || !(st = parse_uint(args[i], out.config.detection.debounce_ms)).ok()) return st; }
        else if (t == "--cooldown-ms") { if (!(st = take()).ok() || !(st = parse_uint(args[i], out.config.detection.cooldown_ms)).ok()) return st; }
        else if (t == "--seed") { if (!(st = take()).ok() || !(st = parse_uint(args[i], out.config.seed)).ok()) return st; }
        else if (t == "--backend") {
            if (!(st = take()).ok()) return st;
            if (args[i] == "real") out.backend = core::BackendKind::Real;
            else if (args[i] == "fake") out.backend = core::BackendKind::Fake;
            else return core::Status::error(core::kErrInvalidArgument, "invalid backend");
        } else if (t == "--scenario") {
            if (!(st = take()).ok()) return st;
            if (args[i] == "static") out.config.scenario = core::FakeScenario::Static;
            else if (args[i] == "human") out.config.scenario = core::FakeScenario::Human;
            else if (args[i] == "pet") out.config.scenario = core::FakeScenario::Pet;
            else if (args[i] == "vibration") out.config.scenario = core::FakeScenario::Vibration;
            else return core::Status::error(core::kErrInvalidArgument, "invalid scenario");
        } else if (t == "--csv") {
            if (!(st = take()).ok()) return st;
            out.csv_path = args[i];
        } else {
            return core::Status::error(core::kErrInvalidArgument, "unknown option: " + t);
        }
    }
    return core::Status::success();
}

} // namespace sonarlock::app
