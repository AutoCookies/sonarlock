#pragma once

#include "sonarlock/core/types.hpp"

#include <vector>

namespace sonarlock::core {

class AutoTuner {
  public:
    explicit AutoTuner(CalibrationSection config);
    void reset();
    void add_sample(double relative_motion);
    bool ready(std::size_t min_samples) const;
    void apply(DetectionSection& detection) const;

  private:
    CalibrationSection config_;
    std::vector<double> samples_;
};

class CalibrationController {
  public:
    CalibrationController(CalibrationSection cal, DetectionSection det);
    void reset();
    CalibrationState state() const;
    void update(double timestamp_sec, double relative_motion, DetectionSection& det_inout);

  private:
    CalibrationSection cal_;
    DetectionSection default_det_;
    CalibrationState state_{CalibrationState::Init};
    AutoTuner tuner_;
};

} // namespace sonarlock::core
