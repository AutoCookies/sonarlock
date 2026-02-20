#pragma once

#include "sonarlock/core/types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace sonarlock::core {

class SineGenerator;

class IDspPipeline {
  public:
    virtual ~IDspPipeline() = default;

    virtual void begin_session(const AudioConfig& config) = 0;
    virtual void process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) = 0;
    [[nodiscard]] virtual RuntimeMetrics metrics() const = 0;
};

class BasicDspPipeline final : public IDspPipeline {
  public:
    explicit BasicDspPipeline(double frequency_hz = 19000.0);
    ~BasicDspPipeline() override;

    void begin_session(const AudioConfig& config) override;
    void process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) override;
    [[nodiscard]] RuntimeMetrics metrics() const override;

  private:
    double frequency_hz_;
    AudioConfig config_{};
    RuntimeMetrics metrics_{};
    std::size_t total_frames_{0};
    std::vector<float> tone_buffer_;
    std::unique_ptr<SineGenerator> generator_;
};

} // namespace sonarlock::core
