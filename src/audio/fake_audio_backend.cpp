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
                                           core::RuntimeMetrics& out_metrics, const std::function<bool()>& should_stop) {
    const auto& a = config.audio;
    if (a.sample_rate_hz <= 0.0 || a.frames_per_buffer == 0) {
        return core::Status::error(core::kErrInvalidArgument, "invalid audio configuration");
    }

    pipeline.begin_session(config);
    const double run_sec = (a.duration_seconds <= 0.0) ? 60.0 : a.duration_seconds;
    const std::size_t total_frames = static_cast<std::size_t>(a.sample_rate_hz * run_sec);

    std::vector<float> input(a.frames_per_buffer, 0.0F);
    std::vector<float> output(a.frames_per_buffer, 0.0F);

    std::mt19937 rng(config.seed == 0 ? seed_ : config.seed);
    std::uniform_real_distribution<float> noise(-0.01F, 0.01F);
    std::uniform_real_distribution<float> jitter(-1.0F, 1.0F);

    std::size_t offset = 0;
    double phase = 0.0;
    while (offset < total_frames && !should_stop()) {
        const std::size_t frames = std::min(a.frames_per_buffer, total_frames - offset);
        input.assign(a.frames_per_buffer, 0.0F);
        for (std::size_t i = 0; i < frames; ++i) {
            const double t = static_cast<double>(offset + i) / a.sample_rate_hz;
            double freq = a.f0_hz;
            double amp = 0.25;
            double extra = 0.0;
            if (config.scenario == core::FakeScenario::Human || scenario_ == core::FakeScenario::Human) {
                const double gate = (t > 0.80 * run_sec && t < 0.98 * run_sec) ? 1.0 : 0.0;
                amp = 0.24;
                extra = gate * 0.45 * std::sin(kTwoPi * (a.f0_hz + 120.0) * t);
            } else if (config.scenario == core::FakeScenario::Pet || scenario_ == core::FakeScenario::Pet) {
                amp = 0.08 + 0.02 * std::sin(kTwoPi * 7.0 * t);
                extra = 0.04 * std::sin(kTwoPi * (a.f0_hz + 25.0 + jitter(rng)) * t);
            } else if (config.scenario == core::FakeScenario::Vibration || scenario_ == core::FakeScenario::Vibration) {
                amp = 0.28 * (1.0 + 0.35 * std::sin(kTwoPi * 8.0 * t));
            }
            phase += kTwoPi * freq / a.sample_rate_hz;
            if (phase >= kTwoPi) phase -= kTwoPi;
            input[i] = static_cast<float>(amp * std::sin(phase) + extra + noise(rng));
        }

        pipeline.process(std::span<const float>(input.data(), frames), std::span<float>(output.data(), frames), offset);
        offset += frames;
    }

    out_metrics = pipeline.metrics();
    out_metrics.xruns = 0;
    return core::Status::success();
}

} // namespace sonarlock::audio
