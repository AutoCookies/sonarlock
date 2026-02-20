#include "sonarlock/app/cli.hpp"

#include "sonarlock/audio/audio_factory.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/logger.hpp"
#include "sonarlock/core/session_controller.hpp"
#include "sonarlock/platform/action_executor.hpp"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
std::atomic_bool g_stop{false};
void signal_handler(int) { g_stop.store(true); }

const char* state_name(sonarlock::core::DetectionState s) {
    using sonarlock::core::DetectionState;
    switch (s) { case DetectionState::Idle: return "IDLE"; case DetectionState::Observing: return "OBSERVING"; case DetectionState::Triggered: return "TRIGGERED"; case DetectionState::Cooldown: return "COOLDOWN"; }
    return "UNKNOWN";
}

void print_help() {
    std::cout << "Usage:\n  sonarlock devices\n  sonarlock calibrate|run|analyze [--backend real|fake] [--config path] ...\n  sonarlock dump-events [--dump-count N]\n";
}

std::string default_config_path() {
#ifdef _WIN32
    const char* app = std::getenv("APPDATA");
    return std::string(app ? app : ".") + "\\sonarlock\\config.json";
#else
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.config/sonarlock/config.json";
#endif
}

std::string extract_config_path(const std::vector<std::string>& args) {
    for (std::size_t i = 0; i + 1 < args.size(); ++i) if (args[i] == "--config") return args[i + 1];
    return {};
}
} // namespace

int main(int argc, char** argv) {
    using namespace sonarlock;
    std::signal(SIGINT, signal_handler);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    app::CommandLine cmd;
    cmd.config_path = extract_config_path(args);
    if (cmd.config_path.empty()) cmd.config_path = default_config_path();
    std::filesystem::create_directories(std::filesystem::path(cmd.config_path).parent_path());
    app::load_config_file(cmd.config_path, cmd.config);

    const auto parse = app::parse_args(args, cmd);
    if (!parse.ok()) { std::cerr << parse.message << '\n'; return parse.code; }
    if (cmd.kind == app::CommandKind::Help) { print_help(); return 0; }

    core::set_global_logger(core::make_console_file_logger(cmd.config.logging));

    if (cmd.kind == app::CommandKind::DumpEvents) {
        std::ifstream in("sonarlock_events.json");
        if (!in) { core::log(core::LogLevel::Warn, "no events journal available"); return 0; }
        std::cout << in.rdbuf();
        return 0;
    }

    auto backend = audio::make_backend(cmd.backend, cmd.config.scenario, cmd.config.seed);
    if (cmd.kind == app::CommandKind::Devices) {
        const auto devices = backend->enumerate_devices();
        if (devices.empty()) { core::log(core::LogLevel::Warn, "no audio devices available"); return core::kErrAudioDeviceUnavailable; }
        for (const auto& d : devices) std::cout << d.id << ": " << d.name << '\n';
        return 0;
    }

    if (cmd.kind == app::CommandKind::Calibrate) {
        cmd.config.actions.manual_disable = true;
        cmd.config.audio.duration_seconds = std::max(10.0, cmd.config.calibration.warmup_seconds + cmd.config.calibration.calibrate_seconds + 1.0);
    }

    core::BasicDspPipeline pipeline;
    core::SessionController controller(*backend);
    core::RuntimeMetrics metrics;
    const auto status = controller.run(cmd.config, pipeline, metrics, [] { return g_stop.load(); });
    if (!status.ok()) { core::log(core::LogLevel::Error, status.message); return status.code; }

    auto executor = platform::make_executor();
    if (metrics.latest_action.type != core::ActionType::None) {
        const auto res = executor->execute(metrics.latest_action);
        core::log(res.ok ? core::LogLevel::Info : core::LogLevel::Warn, std::string("action_result=") + res.message);
    }

    std::ostringstream ss;
    ss << "score=" << metrics.latest_event.score << " confidence=" << metrics.latest_event.confidence
       << " state=" << state_name(metrics.latest_event.state) << " cal=" << static_cast<int>(metrics.latest_event.calibration)
       << " rel=" << metrics.features.relative_motion << " dop=" << metrics.features.doppler_band_energy << " bb=" << metrics.features.baseband_energy
       << " trigger_th=" << cmd.config.detection.trigger_threshold
       << " release_th=" << cmd.config.detection.release_threshold << " triggers=" << metrics.triggered_count;
    core::log(core::LogLevel::Info, ss.str());

    if (cmd.kind == app::CommandKind::Calibrate) {
        std::cout << "recommended_trigger=" << cmd.config.detection.trigger_threshold << " recommended_release=" << cmd.config.detection.release_threshold << '\n';
    }

    if (!cmd.csv_path.empty()) {
        std::ofstream csv(cmd.csv_path);
        csv << "timestamp,state,score,confidence,relative_motion,baseline,doppler,snr\n";
        csv << metrics.latest_event.timestamp_sec << ',' << state_name(metrics.latest_event.state) << ','
            << metrics.latest_event.score << ',' << metrics.latest_event.confidence << ','
            << metrics.features.relative_motion << ',' << metrics.features.baseline_energy << ','
            << metrics.features.doppler_band_energy << ',' << metrics.features.snr_estimate << '\n';
    }

    std::ofstream ev("sonarlock_events.json");
    ev << pipeline.dump_events_json(cmd.dump_count);
    return 0;
}
