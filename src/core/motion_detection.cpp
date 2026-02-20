#include "sonarlock/core/motion_detection.hpp"

#include <algorithm>
#include <cmath>

namespace sonarlock::core {

double DefaultMotionScorer::score(const MotionFeatures& f) const {
    const double e = std::min(1.0, f.relative_motion * 120.0);
    const double ratio = std::min(1.0, f.doppler_band_energy / (f.baseband_energy + 1e-6));
    const double pv = std::min(1.0, std::abs(f.phase_velocity) / 120.0);
    const double snr = std::min(1.0, std::max(0.0, f.snr_estimate) / 24.0);
    return 0.70 * e + 0.15 * ratio + 0.10 * pv + 0.05 * snr;
}

DetectionStateMachine::DetectionStateMachine(DetectionSection config) : config_(config) {}

MotionEvent DetectionStateMachine::update(double score, double confidence, double timestamp_sec, CalibrationState cal_state) {
    if (cal_state != CalibrationState::Armed) {
        state_ = DetectionState::Idle;
        return MotionEvent{state_, cal_state, score, confidence, timestamp_sec};
    }

    if (state_ == DetectionState::Cooldown && timestamp_sec >= cooldown_until_sec_) state_ = DetectionState::Idle;

    if (state_ == DetectionState::Idle) {
        if (score >= config_.release_threshold) {
            state_ = DetectionState::Observing;
            observe_since_sec_ = timestamp_sec;
        }
    } else if (state_ == DetectionState::Observing) {
        if (score < config_.release_threshold) {
            state_ = DetectionState::Idle;
            observe_since_sec_ = -1.0;
        } else if (score >= config_.trigger_threshold &&
                   (timestamp_sec - observe_since_sec_) * 1000.0 >= static_cast<double>(config_.debounce_ms)) {
            state_ = DetectionState::Triggered;
        }
    } else if (state_ == DetectionState::Triggered) {
        state_ = DetectionState::Cooldown;
        cooldown_until_sec_ = timestamp_sec + static_cast<double>(config_.cooldown_ms) / 1000.0;
    }

    return MotionEvent{state_, cal_state, score, confidence, timestamp_sec};
}

MotionDetector::MotionDetector(DetectionSection config, std::unique_ptr<IMotionScorer> scorer)
    : config_(config), scorer_(std::move(scorer)), fsm_(config) {}

void DetectionStateMachine::set_config(DetectionSection config) { config_ = config; }

MotionEvent MotionDetector::evaluate(const MotionFeatures& features, double timestamp_sec, CalibrationState cal_state) {
    const double score = scorer_->score(features);
    const double confidence = std::clamp(score, 0.0, 1.0);
    return fsm_.update(score, confidence, timestamp_sec, cal_state);
}

void MotionDetector::set_detection_config(DetectionSection config) {
    config_ = config;
    fsm_.set_config(config);
}

} // namespace sonarlock::core
