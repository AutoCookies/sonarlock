#include "sonarlock/core/action_policy.hpp"

namespace sonarlock::core {

ActionRequest DefaultActionPolicy::map(const MotionEvent& event, ActionMode mode) {
    if (event.state != DetectionState::Triggered) return {};
    if (mode == ActionMode::Lock) return {ActionType::LockScreen, event.timestamp_sec, "triggered_motion"};
    if (mode == ActionMode::Notify) return {ActionType::Notify, event.timestamp_sec, "triggered_motion"};
    return {ActionType::Beep, event.timestamp_sec, "triggered_motion"};
}

ActionSafetyController::ActionSafetyController(DetectionSection config) : cfg_(config) {}

bool ActionSafetyController::allow(const ActionRequest& req, bool manual_disable, double now_sec) {
    if (manual_disable || req.type == ActionType::None) return false;
    if (now_sec * 1000.0 < static_cast<double>(cfg_.arming_delay_ms)) return false;

    while (!lock_times_.empty() && now_sec - lock_times_.front() > 60.0) lock_times_.pop_front();

    if (req.type == ActionType::LockScreen) {
        if (now_sec < lock_cooldown_until_) return false;
        if (lock_times_.size() >= cfg_.max_locks_per_minute) return false;
        lock_times_.push_back(now_sec);
        lock_cooldown_until_ = now_sec + static_cast<double>(cfg_.lock_cooldown_ms) / 1000.0;
    }
    return true;
}

} // namespace sonarlock::core
