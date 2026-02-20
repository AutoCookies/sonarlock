#include "sonarlock/core/dsp_primitives.hpp"

#include <cmath>

namespace sonarlock::core {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

Nco::Nco(double sample_rate_hz, double frequency_hz) : sample_rate_hz_(sample_rate_hz), frequency_hz_(frequency_hz) {}

std::pair<double, double> Nco::next() {
    const auto c = std::cos(phase_);
    const auto s = std::sin(phase_);
    phase_ += kTwoPi * frequency_hz_ / sample_rate_hz_;
    if (phase_ >= kTwoPi) phase_ -= kTwoPi;
    return {c, s};
}

IirLowPass::IirLowPass(double sample_rate_hz, double cutoff_hz) {
    const double rc = 1.0 / (kTwoPi * cutoff_hz);
    const double dt = 1.0 / sample_rate_hz;
    alpha_ = dt / (rc + dt);
}

double IirLowPass::process(double x) {
    y_ += alpha_ * (x - y_);
    return y_;
}

double PhaseTracker::unwrap(double i, double q) {
    const double wrapped = std::atan2(q, i);
    if (!initialized_) {
        initialized_ = true;
        last_wrapped_ = wrapped;
        unwrapped_ = wrapped;
        return unwrapped_;
    }
    double d = wrapped - last_wrapped_;
    constexpr double kPi = 3.14159265358979323846;
    if (d > kPi) d -= kTwoPi;
    if (d < -kPi) d += kTwoPi;
    unwrapped_ += d;
    last_wrapped_ = wrapped;
    return unwrapped_;
}

} // namespace sonarlock::core
