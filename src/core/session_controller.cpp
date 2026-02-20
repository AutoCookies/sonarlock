#include "sonarlock/core/session_controller.hpp"

namespace sonarlock::core {

SessionController::SessionController(IAudioBackend& backend) : backend_(backend) {}

SessionState SessionController::state() const { return state_; }

Status SessionController::run(const AudioConfig& config, IDspPipeline& pipeline, RuntimeMetrics& out_metrics,
                              const std::function<bool()>& should_stop) {
    state_ = SessionState::Running;
    const auto status = backend_.run_session(config, pipeline, out_metrics, should_stop);
    state_ = status.ok() ? SessionState::Stopped : SessionState::Error;
    return status;
}

} // namespace sonarlock::core
