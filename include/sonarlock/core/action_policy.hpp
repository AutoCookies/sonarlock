#pragma once

#include "sonarlock/core/types.hpp"

#include <deque>
#include <memory>

namespace sonarlock::core {

class IActionPolicy {
  public:
    virtual ~IActionPolicy() = default;
    virtual ActionRequest map(const MotionEvent& event, ActionMode mode) = 0;
};

class DefaultActionPolicy final : public IActionPolicy {
  public:
    ActionRequest map(const MotionEvent& event, ActionMode mode) override;
};

class ActionSafetyController {
  public:
    explicit ActionSafetyController(DetectionSection config);
    bool allow(const ActionRequest& req, bool manual_disable, double now_sec);

  private:
    DetectionSection cfg_;
    double lock_cooldown_until_{0.0};
    std::deque<double> lock_times_;
};

} // namespace sonarlock::core
