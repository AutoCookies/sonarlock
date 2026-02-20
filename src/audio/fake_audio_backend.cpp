#include "sonarlock/audio/fake_audio_backend.hpp"

#include <cmath>
#include <random>

namespace sonarlock::audio {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

FakeAudioBackend::FakeAudioBackend(core::FakeScenario scenario, std::uint32_t seed) : scenario_(scenario), seed_(seed) {}

std::vector<core::AudioDeviceInfo> FakeAudioBackend::enumerate_devices() const {
    return {{0, "Fake Loopback Device", 1, 1, 48000.0}};
}

core::Status FakeAudioBackend::run_session(const core::AudioConfig& config, core::IDspPipeline& pipeline,
                                           core::RuntimeMetrics& out_metrics) {
    const auto& dsp = config.dsp;
    if (dsp.sample_rate_hz <= 0.0 || dsp.frames_per_buffer == 0 || dsp.duration_seconds <= 0.0) {
        return core::Status::error(core::kErrInvalidArgument, "invalid audio configuration");
    }

    pipeline.begin_session(config);

    const std::size_t total_frames = static_cast<std::size_t>(dsp.sample_rate_hz * dsp.duration_seconds);
    std::vector<float> input(dsp.frames_per_buffer, 0.0F);
    std::vector<float> output(dsp.frames_per_buffer, 0.0F);

    std::mt19937 rng(config.seed == 0 ? seed_ : config.seed);
    std::uniform_real_distribution<float> noise(-0.01F, 0.01F);
    std::uniform_real_distribution<float> jitter(-1.0F, 1.0F);

    std::size_t offset = 0;
    double phase = 0.0;
    while (offset < total_frames) {
        const std::size_t frames = std::min(dsp.frames_per_buffer, total_frames - offset);
        input.assign(dsp.frames_per_buffer, 0.0F);
        for (std::size_t i = 0; i < frames; ++i) {
            const double t = static_cast<double>(offset + i) / dsp.sample_rate_hz;
            const double x = std::sin(3.14159265358979323846 * t / dsp.duration_seconds);
            double freq = dsp.f0_hz;
            double amp = 0.25;
            if (config.scenario == core::FakeScenario::Human || scenario_ == core::FakeScenario::Human) {
                const double gate = (t > 0.35 * dsp.duration_seconds && t < 0.75 * dsp.duration_seconds) ? 1.0 : 0.0;
                freq += 120.0 * gate;
                amp = 0.26 + 0.12 * gate;
            } else if (config.scenario == core::FakeScenario::Pet || scenario_ == core::FakeScenario::Pet) {
                freq += 12.0 * std::sin(2.0 * 3.14159265358979323846 * 6.0 * t) + 2.0 * jitter(rng);
                amp = 0.07 + 0.02 * std::sin(2.0 * 3.14159265358979323846 * 7.0 * t);
            } else if (config.scenario == core::FakeScenario::Vibration || scenario_ == core::FakeScenario::Vibration) {
                amp = 0.28 * (1.0 + 0.35 * std::sin(2.0 * 3.14159265358979323846 * 8.0 * t));
            }
            phase += kTwoPi * freq / dsp.sample_rate_hz;
            if (phase >= kTwoPi) phase -= kTwoPi;
            input[i] = static_cast<float>(amp * std::sin(phase) + noise(rng));
        }

        pipeline.process(std::span<const float>(input.data(), frames), std::span<float>(output.data(), frames), offset);
        offset += frames;
    }

    out_metrics = pipeline.metrics();
    out_metrics.xruns = 0;
    return core::Status::success();
}

} // namespace sonarlock::audio
