#pragma once

#include <utility>

namespace sonarlock::core {

class Nco {
  public:
    Nco(double sample_rate_hz, double frequency_hz);
    std::pair<double, double> next(); // cos, sin

  private:
    double sample_rate_hz_;
    double frequency_hz_;
    double phase_{0.0};
};

class IirLowPass {
  public:
    IirLowPass(double sample_rate_hz, double cutoff_hz);
    double process(double x);

  private:
    double alpha_;
    double y_{0.0};
};

class PhaseTracker {
  public:
    double unwrap(double i, double q);

  private:
    double last_wrapped_{0.0};
    double unwrapped_{0.0};
    bool initialized_{false};
};

} // namespace sonarlock::core
