#include "sonarlock/audio/fake_audio_backend.hpp"

#include <cmath>
#include <random>

namespace sonarlock::audio {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

FakeAudioBackend::FakeAudioBackend(core::FakeInputMode mode, std::uint32_t seed) : mode_(mode), seed_(seed) {}

std::vector<core::AudioDeviceInfo> FakeAudioBackend::enumerate_devices() const {
    return {{0, "Fake Loopback Device", 1, 1, 48000.0}};
}

core::Status FakeAudioBackend::run_session(const core::AudioConfig& config, core::IDspPipeline& pipeline,
                                           core::RuntimeMetrics& out_metrics) {
    if (config.sample_rate_hz <= 0.0 || config.frames_per_buffer == 0 || config.duration_seconds <= 0.0) {
        return core::Status::error(core::kErrInvalidArgument, "invalid audio configuration");
    }

    pipeline.begin_session(config);

    const std::size_t total_frames = static_cast<std::size_t>(config.sample_rate_hz * config.duration_seconds);
    std::vector<float> input(config.frames_per_buffer, 0.0F);
    std::vector<float> output(config.frames_per_buffer, 0.0F);

    std::mt19937 rng(seed_);
    std::uniform_real_distribution<float> noise(-0.02F, 0.02F);

    std::size_t offset = 0;
    double phase = 0.0;
    const double phase_inc = kTwoPi * config.tone_frequency_hz / config.sample_rate_hz;
    while (offset < total_frames) {
        const std::size_t frames = std::min(config.frames_per_buffer, total_frames - offset);
        input.assign(config.frames_per_buffer, 0.0F);
        if (mode_ == core::FakeInputMode::ToneWithNoise) {
            for (std::size_t i = 0; i < frames; ++i) {
                input[i] = static_cast<float>(0.3 * std::sin(phase) + noise(rng));
                phase += phase_inc;
                if (phase >= kTwoPi) {
                    phase -= kTwoPi;
                }
            }
        }

        pipeline.process(std::span<const float>(input.data(), frames), std::span<float>(output.data(), frames), offset);
        offset += frames;
    }

    out_metrics = pipeline.metrics();
    out_metrics.xruns = 0;
    return core::Status::success();
}

} // namespace sonarlock::audio
