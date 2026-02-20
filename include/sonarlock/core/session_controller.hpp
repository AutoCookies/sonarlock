#pragma once

#include "sonarlock/core/audio_backend.hpp"

#include <functional>

namespace sonarlock::core {

class SessionController {
  public:
    explicit SessionController(IAudioBackend& backend);

    [[nodiscard]] SessionState state() const;
    [[nodiscard]] Status run(const AudioConfig& config, IDspPipeline& pipeline, RuntimeMetrics& out_metrics,
                             const std::function<bool()>& should_stop);

  private:
    IAudioBackend& backend_;
    SessionState state_{SessionState::Idle};
};

} // namespace sonarlock::core
