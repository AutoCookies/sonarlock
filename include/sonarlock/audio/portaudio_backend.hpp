#pragma once

#include "sonarlock/core/audio_backend.hpp"

namespace sonarlock::audio {

class PortAudioBackend final : public core::IAudioBackend {
  public:
    [[nodiscard]] std::vector<core::AudioDeviceInfo> enumerate_devices() const override;
    core::Status run_session(const core::AudioConfig& config, core::IDspPipeline& pipeline,
                             core::RuntimeMetrics& out_metrics) override;
};

} // namespace sonarlock::audio
