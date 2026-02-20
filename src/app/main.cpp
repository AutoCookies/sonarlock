#include "sonarlock/app/cli.hpp"

#include "sonarlock/audio/audio_factory.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/logger.hpp"
#include "sonarlock/core/session_controller.hpp"

#include <iostream>
#include <sstream>

namespace {

void print_help() {
    std::cout << "Usage:\n"
              << "  sonarlock devices [--backend real|fake]\n"
              << "  sonarlock run [--duration sec] [--freq hz] [--samplerate hz] [--frames N] [--backend real|fake]"
              << " [--fake-input silence|tone-noise]\n";
}

}

int main(int argc, char** argv) {
    using namespace sonarlock;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

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

    auto backend = audio::make_backend(cmd.backend, cmd.fake_mode);

    if (cmd.kind == app::CommandKind::Devices) {
        const auto devices = backend->enumerate_devices();
        if (devices.empty()) {
            core::log(core::LogLevel::Warn, "no audio devices available");
            return core::kErrAudioDeviceUnavailable;
        }
        for (const auto& device : devices) {
            std::cout << device.id << ": " << device.name << " in=" << device.max_input_channels
                      << " out=" << device.max_output_channels << " default_rate=" << device.default_sample_rate
                      << '\n';
        }
        return 0;
    }

    core::BasicDspPipeline pipeline(cmd.config.tone_frequency_hz);
    core::SessionController controller(*backend);
    core::RuntimeMetrics metrics;
    const auto status = controller.run(cmd.config, pipeline, metrics);
    if (!status.ok()) {
        core::log(core::LogLevel::Error, status.message);
        return status.code;
    }

    std::ostringstream ss;
    ss << "session complete sample_rate=" << metrics.sample_rate_hz << " frames=" << metrics.frames_per_buffer
       << " peak=" << metrics.peak_level << " rms=" << metrics.rms_level << " dc=" << metrics.dc_offset
       << " xruns=" << metrics.xruns << " callbacks=" << metrics.callbacks;
    core::log(core::LogLevel::Info, ss.str());
    return 0;
}
