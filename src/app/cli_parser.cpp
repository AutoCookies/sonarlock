#include "sonarlock/app/cli.hpp"

#include <fstream>
#include <regex>
#include <type_traits>

namespace sonarlock::app {

namespace {

template <typename T>
core::Status parse_num(const std::string& value, T& out) {
    try {
        if constexpr (std::is_same_v<T, std::size_t>) out = static_cast<std::size_t>(std::stoull(value));
        else if constexpr (std::is_same_v<T, std::uint32_t>) out = static_cast<std::uint32_t>(std::stoul(value));
        else out = static_cast<T>(std::stod(value));
    } catch (...) {
        return core::Status::error(core::kErrInvalidArgument, "bad number: " + value);
    }
    return core::Status::success();
}

std::string find_json_string(const std::string& text, const std::string& key) {
    std::regex r("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch m;
    if (std::regex_search(text, m, r)) return m[1];
    return {};
}

double find_json_number(const std::string& text, const std::string& key, double def) {
    std::regex r("\"" + key + "\"\\s*:\\s*([-+0-9.eE]+)");
    std::smatch m;
    if (std::regex_search(text, m, r)) return std::stod(m[1]);
    return def;
}

} // namespace

core::Status load_config_file(const std::string& path, core::AudioConfig& cfg) {
    if (path.empty()) return core::Status::success();
    std::ifstream in(path);
    if (!in) return core::Status::error(core::kErrInvalidArgument, "cannot open config: " + path);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    cfg.audio.sample_rate_hz = find_json_number(text, "sample_rate_hz", cfg.audio.sample_rate_hz);
    cfg.audio.frames_per_buffer = static_cast<std::size_t>(find_json_number(text, "frames_per_buffer", static_cast<double>(cfg.audio.frames_per_buffer)));
    cfg.audio.duration_seconds = find_json_number(text, "duration_seconds", cfg.audio.duration_seconds);
    cfg.audio.f0_hz = find_json_number(text, "f0_hz", cfg.audio.f0_hz);
    cfg.dsp.lp_cutoff_hz = find_json_number(text, "lp_cutoff_hz", cfg.dsp.lp_cutoff_hz);
    cfg.dsp.doppler_band_low_hz = find_json_number(text, "doppler_band_low_hz", cfg.dsp.doppler_band_low_hz);
    cfg.dsp.doppler_band_high_hz = find_json_number(text, "doppler_band_high_hz", cfg.dsp.doppler_band_high_hz);
    cfg.detection.trigger_threshold = find_json_number(text, "trigger_threshold", cfg.detection.trigger_threshold);
    cfg.detection.release_threshold = find_json_number(text, "release_threshold", cfg.detection.release_threshold);
    cfg.detection.debounce_ms = static_cast<std::uint32_t>(find_json_number(text, "debounce_ms", cfg.detection.debounce_ms));
    cfg.detection.cooldown_ms = static_cast<std::uint32_t>(find_json_number(text, "cooldown_ms", cfg.detection.cooldown_ms));

    const auto mode = find_json_string(text, "action_mode");
    if (mode == "lock") cfg.actions.mode = core::ActionMode::Lock;
    else if (mode == "notify") cfg.actions.mode = core::ActionMode::Notify;
    else if (mode == "soft") cfg.actions.mode = core::ActionMode::Soft;

    return core::Status::success();
}

core::Status parse_args(const std::vector<std::string>& args, CommandLine& out) {
    if (args.empty()) return core::Status::success();

    if (args[0] == "devices") out.kind = CommandKind::Devices;
    else if (args[0] == "run") out.kind = CommandKind::Run;
    else if (args[0] == "analyze") out.kind = CommandKind::Analyze;
    else if (args[0] == "calibrate") out.kind = CommandKind::Calibrate;
    else if (args[0] == "dump-events") out.kind = CommandKind::DumpEvents;
    else return core::Status::error(core::kErrInvalidArgument, "unknown command: " + args[0]);

    for (std::size_t i = 1; i < args.size(); ++i) {
        const auto& t = args[i];
        auto take = [&]() -> core::Status { if (i + 1 >= args.size()) return core::Status::error(core::kErrInvalidArgument, "missing value for " + t); ++i; return core::Status::success(); };
        core::Status st;

        if (t == "--config") { if (!(st = take()).ok()) return st; out.config_path = args[i]; }
        else if (t == "--duration") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.audio.duration_seconds)).ok()) return st; }
        else if (t == "--f0" || t == "--freq") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.audio.f0_hz)).ok()) return st; }
        else if (t == "--samplerate") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.audio.sample_rate_hz)).ok()) return st; }
        else if (t == "--frames") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.audio.frames_per_buffer)).ok()) return st; }
        else if (t == "--lp-cutoff") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.dsp.lp_cutoff_hz)).ok()) return st; }
        else if (t == "--band-low") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.dsp.doppler_band_low_hz)).ok()) return st; }
        else if (t == "--band-high") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.dsp.doppler_band_high_hz)).ok()) return st; }
        else if (t == "--trigger-th") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.detection.trigger_threshold)).ok()) return st; }
        else if (t == "--release-th") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.detection.release_threshold)).ok()) return st; }
        else if (t == "--debounce-ms") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.detection.debounce_ms)).ok()) return st; }
        else if (t == "--cooldown-ms") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.config.detection.cooldown_ms)).ok()) return st; }
        else if (t == "--backend") { if (!(st = take()).ok()) return st; out.backend = (args[i] == "real") ? core::BackendKind::Real : core::BackendKind::Fake; }
        else if (t == "--scenario") {
            if (!(st = take()).ok()) return st;
            if (args[i] == "static") out.config.scenario = core::FakeScenario::Static;
            else if (args[i] == "human") out.config.scenario = core::FakeScenario::Human;
            else if (args[i] == "pet") out.config.scenario = core::FakeScenario::Pet;
            else if (args[i] == "vibration") out.config.scenario = core::FakeScenario::Vibration;
            else return core::Status::error(core::kErrInvalidArgument, "invalid scenario");
        } else if (t == "--csv") { if (!(st = take()).ok()) return st; out.csv_path = args[i]; }
        else if (t == "--json") { out.json_output = true; }
        else if (t == "--dump-count") { if (!(st = take()).ok() || !(st = parse_num(args[i], out.dump_count)).ok()) return st; }
        else if (t == "--daemon") { out.config.daemon_mode = true; }
        else if (t == "--no-calibration") { out.config.calibration.enabled = false; }
        else if (t == "--disable-actions") { out.config.actions.manual_disable = true; }
        else if (t == "--action") {
            if (!(st = take()).ok()) return st;
            if (args[i] == "lock") out.config.actions.mode = core::ActionMode::Lock;
            else if (args[i] == "notify") out.config.actions.mode = core::ActionMode::Notify;
            else out.config.actions.mode = core::ActionMode::Soft;
        }
        else return core::Status::error(core::kErrInvalidArgument, "unknown option: " + t);
    }
    return core::Status::success();
}

} // namespace sonarlock::app
