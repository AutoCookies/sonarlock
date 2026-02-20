#pragma once

#include <cstddef>
#include <vector>

namespace sonarlock::core {

class SineGenerator {
  public:
    SineGenerator(double sample_rate_hz, double frequency_hz, double fade_ms = 20.0);

    void set_frequency(double frequency_hz);
    void reset();

    void generate(std::vector<float>& out, std::size_t total_frames, std::size_t frame_offset);

  private:
    double sample_rate_hz_;
    double frequency_hz_;
    double phase_{0.0};
    std::size_t fade_samples_{0};
};

} // namespace sonarlock::core
