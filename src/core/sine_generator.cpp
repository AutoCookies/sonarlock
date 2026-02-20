#include "sonarlock/core/sine_generator.hpp"

#include <algorithm>
#include <cmath>

namespace sonarlock::core {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

SineGenerator::SineGenerator(double sample_rate_hz, double frequency_hz, double fade_ms)
    : sample_rate_hz_(sample_rate_hz), frequency_hz_(frequency_hz) {
    fade_samples_ = static_cast<std::size_t>((fade_ms / 1000.0) * sample_rate_hz_);
}

void SineGenerator::set_frequency(double frequency_hz) { frequency_hz_ = frequency_hz; }

void SineGenerator::reset() { phase_ = 0.0; }

void SineGenerator::generate(std::vector<float>& out, std::size_t total_frames, std::size_t frame_offset) {
    const double phase_inc = kTwoPi * frequency_hz_ / sample_rate_hz_;
    for (std::size_t i = 0; i < out.size(); ++i) {
        const std::size_t absolute_frame = frame_offset + i;
        double env = 1.0;
        if (fade_samples_ > 0) {
            if (absolute_frame < fade_samples_) {
                env = static_cast<double>(absolute_frame) / static_cast<double>(fade_samples_);
            }
            const std::size_t remaining = total_frames > absolute_frame ? (total_frames - absolute_frame) : 0;
            if (remaining < fade_samples_) {
                env = std::min(env, static_cast<double>(remaining) / static_cast<double>(fade_samples_));
            }
        }

        out[i] = static_cast<float>(std::sin(phase_) * env);
        phase_ += phase_inc;
        if (phase_ >= kTwoPi) {
            phase_ -= kTwoPi;
        }
    }
}

} // namespace sonarlock::core
