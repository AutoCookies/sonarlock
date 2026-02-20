#include "sonarlock/app/cli.hpp"
#include "sonarlock/audio/fake_audio_backend.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/sine_generator.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool approx(double a, double b, double eps = 1e-3) { return std::abs(a - b) <= eps; }

bool test_sine_generator() {
    sonarlock::core::SineGenerator gen(48000.0, 19000.0);
    std::vector<float> first(256, 0.0F);
    std::vector<float> second(256, 0.0F);
    gen.generate(first, 48000, 0);
    gen.generate(second, 48000, 256);

    for (float sample : first) {
        if (sample > 1.0F || sample < -1.0F) return false;
    }
    return std::abs(second.front() - first.back()) < 1.0;
}

bool test_dsp_metrics() {
    sonarlock::core::BasicDspPipeline pipeline(19000.0);
    sonarlock::core::AudioConfig config;
    config.duration_seconds = 1.0;
    pipeline.begin_session(config);

    std::vector<float> input{1.0F, -1.0F, 1.0F, -1.0F};
    std::vector<float> output(4, 0.0F);
    pipeline.process(input, output, 0);

    const auto metrics = pipeline.metrics();
    return approx(metrics.peak_level, 1.0) && approx(metrics.rms_level, 1.0) && approx(metrics.dc_offset, 0.0, 1e-5);
}

bool test_fake_backend() {
    sonarlock::audio::FakeAudioBackend backend(sonarlock::core::FakeInputMode::ToneWithNoise, 42);
    sonarlock::core::BasicDspPipeline pipeline;
    sonarlock::core::AudioConfig config;
    config.sample_rate_hz = 48000;
    config.frames_per_buffer = 256;
    config.duration_seconds = 1.0;

    sonarlock::core::RuntimeMetrics metrics;
    auto status = backend.run_session(config, pipeline, metrics);
    return status.ok() && metrics.frames_processed == 48000 && metrics.callbacks == 188 && metrics.peak_level > 0.2F;
}

bool test_cli_parse_devices() {
    sonarlock::app::CommandLine cmd;
    auto status = sonarlock::app::parse_args({"devices", "--backend", "fake"}, cmd);
    return status.ok() && cmd.kind == sonarlock::app::CommandKind::Devices &&
           cmd.backend == sonarlock::core::BackendKind::Fake;
}

bool test_cli_parse_run() {
    sonarlock::app::CommandLine cmd;
    auto status = sonarlock::app::parse_args({"run", "--duration", "2", "--freq", "18000", "--samplerate", "44100",
                                              "--frames", "128", "--backend", "fake", "--fake-input", "tone-noise"},
                                             cmd);
    return status.ok() && cmd.kind == sonarlock::app::CommandKind::Run && approx(cmd.config.duration_seconds, 2.0) &&
           approx(cmd.config.tone_frequency_hz, 18000.0) && approx(cmd.config.sample_rate_hz, 44100.0) &&
           cmd.config.frames_per_buffer == 128 && cmd.fake_mode == sonarlock::core::FakeInputMode::ToneWithNoise;
}

bool test_cli_rejects_unknown_option() {
    sonarlock::app::CommandLine cmd;
    auto status = sonarlock::app::parse_args({"run", "--bad"}, cmd);
    return !status.ok();
}

} // namespace

int main() {
    struct TestCase {
        std::string name;
        bool (*fn)();
    };

    const std::vector<TestCase> tests = {
        {"sine_generator", test_sine_generator},
        {"dsp_metrics", test_dsp_metrics},
        {"fake_backend", test_fake_backend},
        {"cli_devices", test_cli_parse_devices},
        {"cli_run", test_cli_parse_run},
        {"cli_invalid", test_cli_rejects_unknown_option},
    };

    for (const auto& test : tests) {
        if (!test.fn()) {
            std::cerr << "FAILED: " << test.name << '\n';
            return 1;
        }
    }

    std::cout << "All tests passed: " << tests.size() << '\n';
    return 0;
}
