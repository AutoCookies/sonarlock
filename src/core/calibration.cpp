#include "sonarlock/core/calibration.hpp"

#include <algorithm>
#include <cmath>

namespace sonarlock::core {

AutoTuner::AutoTuner(CalibrationSection config) : config_(config) {}

void AutoTuner::reset() { samples_.clear(); }

void AutoTuner::add_sample(double relative_motion) { samples_.push_back(relative_motion); }

bool AutoTuner::ready(std::size_t min_samples) const { return samples_.size() >= min_samples; }

void AutoTuner::apply(DetectionSection& detection) const {
    if (samples_.empty()) return;
    auto s = samples_;
    std::sort(s.begin(), s.end());
    const double median = s[s.size() / 2];
    std::vector<double> dev(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) dev[i] = std::abs(s[i] - median);
    std::sort(dev.begin(), dev.end());
    const double mad = dev[dev.size() / 2] + 1e-6;

    const double trig = std::clamp(median + config_.trigger_k * mad, config_.min_threshold, config_.max_threshold);
    const double rel = std::clamp(median + config_.release_k * mad, config_.min_threshold * 0.5, trig * 0.95);
    detection.trigger_threshold = trig;
    detection.release_threshold = rel;
}

CalibrationController::CalibrationController(CalibrationSection cal, DetectionSection det)
    : cal_(cal), default_det_(det), tuner_(cal) {}

void CalibrationController::reset() {
    state_ = CalibrationState::Init;
    tuner_.reset();
}

CalibrationState CalibrationController::state() const { return state_; }

void CalibrationController::update(double timestamp_sec, double relative_motion, DetectionSection& det_inout) {
    if (!cal_.enabled) {
        state_ = CalibrationState::Armed;
        return;
    }

    if (state_ == CalibrationState::Init) {
        state_ = CalibrationState::Warmup;
    }
    if (state_ == CalibrationState::Warmup && timestamp_sec >= cal_.warmup_seconds) {
        state_ = CalibrationState::Calibrating;
    }
    if (state_ == CalibrationState::Calibrating) {
        tuner_.add_sample(relative_motion);
        if (timestamp_sec >= cal_.warmup_seconds + cal_.calibrate_seconds && tuner_.ready(64)) {
            det_inout = default_det_;
            tuner_.apply(det_inout);
            state_ = CalibrationState::Armed;
        }
    }
}

} // namespace sonarlock::core
