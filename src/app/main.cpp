#include "sonarlock/app/cli.hpp"

#include "sonarlock/audio/audio_factory.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/logger.hpp"
#include "sonarlock/core/session_controller.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {

const char* state_name(sonarlock::core::DetectionState s) {
    using sonarlock::core::DetectionState;
    switch (s) {
        case DetectionState::Idle: return "IDLE";
        case DetectionState::Observing: return "OBSERVING";
        case DetectionState::Triggered: return "TRIGGERED";
        case DetectionState::Cooldown: return "COOLDOWN";
    }
    return "UNKNOWN";
}

void print_help() {
    std::cout << "Usage:\n"
              << "  sonarlock devices [--backend real|fake]\n"
              << "  sonarlock run|analyze [--backend real|fake] [--scenario static|human|pet|vibration] [--duration sec] [--f0 hz] [--lp-cutoff hz] [--band-low hz] [--band-high hz]\n"
              << "                       [--trigger-th v] [--release-th v] [--debounce-ms ms] [--cooldown-ms ms] [--csv out.csv]\n";
}

}

int main(int argc, char** argv) {
    using namespace sonarlock;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    app::CommandLine cmd;
    const auto parse = app::parse_args(args, cmd);
    if (!parse.ok()) {
        core::log(core::LogLevel::Error, parse.message);
        return parse.code;
    }

    if (cmd.kind == app::CommandKind::Help) {
        print_help();
        return 0;
    }

    auto backend = audio::make_backend(cmd.backend, cmd.config.scenario, cmd.config.seed);

    if (cmd.kind == app::CommandKind::Devices) {
        const auto devices = backend->enumerate_devices();
        if (devices.empty()) {
            core::log(core::LogLevel::Warn, "no audio devices available");
            return core::kErrAudioDeviceUnavailable;
        }
        for (const auto& d : devices) {
            std::cout << d.id << ": " << d.name << " in=" << d.max_input_channels << " out=" << d.max_output_channels << '\n';
        }
        return 0;
    }

    core::BasicDspPipeline pipeline;
    core::SessionController controller(*backend);
    core::RuntimeMetrics metrics;
    const auto status = controller.run(cmd.config, pipeline, metrics);
    if (!status.ok()) {
        core::log(core::LogLevel::Error, status.message);
        return status.code;
    }

    std::ostringstream ss;
    ss << "sample_rate=" << metrics.sample_rate_hz << " frames=" << metrics.frames_per_buffer
       << " peak=" << metrics.peak_level << " rms=" << metrics.rms_level << " dc=" << metrics.dc_offset
       << " bb=" << metrics.features.baseband_energy << " doppler=" << metrics.features.doppler_band_energy
       << " pv=" << metrics.features.phase_velocity << " snr=" << metrics.features.snr_estimate
       << " score=" << metrics.latest_event.score << " confidence=" << metrics.latest_event.confidence
       << " state=" << state_name(metrics.latest_event.state) << " triggers=" << metrics.triggered_count;
    core::log(core::LogLevel::Info, ss.str());

    if (cmd.kind == app::CommandKind::Analyze && !cmd.csv_path.empty()) {
        std::ofstream csv(cmd.csv_path);
        csv << "timestamp,state,score,confidence,baseband_energy,doppler_energy,phase_velocity,snr\n";
        csv << metrics.latest_event.timestamp_sec << ',' << state_name(metrics.latest_event.state) << ','
            << metrics.latest_event.score << ',' << metrics.latest_event.confidence << ','
            << metrics.features.baseband_energy << ',' << metrics.features.doppler_band_energy << ','
            << metrics.features.phase_velocity << ',' << metrics.features.snr_estimate << '\n';
    }

    return 0;
}
