#pragma once

#include "sonarlock/core/audio_backend.hpp"

namespace sonarlock::core {

class SessionController {
  public:
    explicit SessionController(IAudioBackend& backend);

    [[nodiscard]] SessionState state() const;
    [[nodiscard]] Status run(const AudioConfig& config, IDspPipeline& pipeline, RuntimeMetrics& out_metrics);

  private:
    IAudioBackend& backend_;
    SessionState state_{SessionState::Idle};
};

} // namespace sonarlock::core
