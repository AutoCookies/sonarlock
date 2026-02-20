#pragma once

#include "sonarlock/core/types.hpp"

#include <memory>

namespace sonarlock::core {

class IMotionScorer {
  public:
    virtual ~IMotionScorer() = default;
    virtual double score(const MotionFeatures& features) const = 0;
};

class DefaultMotionScorer final : public IMotionScorer {
  public:
    double score(const MotionFeatures& features) const override;
};

class DetectionStateMachine {
  public:
    explicit DetectionStateMachine(DetectionSection config);
    MotionEvent update(double score, double confidence, double timestamp_sec, CalibrationState cal_state);
    void set_config(DetectionSection config);

  private:
    DetectionSection config_;
    DetectionState state_{DetectionState::Idle};
    double observe_since_sec_{-1.0};
    double cooldown_until_sec_{0.0};
};

class MotionDetector {
  public:
    MotionDetector(DetectionSection config, std::unique_ptr<IMotionScorer> scorer);
    MotionEvent evaluate(const MotionFeatures& features, double timestamp_sec, CalibrationState cal_state);
    void set_detection_config(DetectionSection config);

  private:
    DetectionSection config_;
    std::unique_ptr<IMotionScorer> scorer_;
    DetectionStateMachine fsm_;
};

} // namespace sonarlock::core
