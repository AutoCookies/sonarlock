#pragma once

#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/types.hpp"

#include <functional>
#include <vector>

namespace sonarlock::core {

class IAudioBackend {
  public:
    virtual ~IAudioBackend() = default;

    [[nodiscard]] virtual std::vector<AudioDeviceInfo> enumerate_devices() const = 0;
    virtual Status run_session(const AudioConfig& config, IDspPipeline& pipeline, RuntimeMetrics& out_metrics,
                               const std::function<bool()>& should_stop) = 0;
};

} // namespace sonarlock::core
