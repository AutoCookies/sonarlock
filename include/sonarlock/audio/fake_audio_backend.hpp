#pragma once

#include "sonarlock/core/audio_backend.hpp"

namespace sonarlock::audio {

class FakeAudioBackend final : public core::IAudioBackend {
  public:
    explicit FakeAudioBackend(core::FakeScenario scenario = core::FakeScenario::Static, std::uint32_t seed = 7);

    [[nodiscard]] std::vector<core::AudioDeviceInfo> enumerate_devices() const override;
    core::Status run_session(const core::AudioConfig& config, core::IDspPipeline& pipeline,
                             core::RuntimeMetrics& out_metrics, const std::function<bool()>& should_stop) override;

  private:
    core::FakeScenario scenario_;
    std::uint32_t seed_;
};

} // namespace sonarlock::audio
