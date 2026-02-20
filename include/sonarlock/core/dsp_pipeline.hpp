#pragma once

#include "sonarlock/core/motion_detection.hpp"
#include "sonarlock/core/types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace sonarlock::core {

class SineGenerator;
class Nco;
class IirLowPass;
class PhaseTracker;

class IDspPipeline {
  public:
    virtual ~IDspPipeline() = default;
    virtual void begin_session(const AudioConfig& config) = 0;
    virtual void process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) = 0;
    [[nodiscard]] virtual RuntimeMetrics metrics() const = 0;
};

class BasicDspPipeline final : public IDspPipeline {
  public:
    BasicDspPipeline();
    ~BasicDspPipeline() override;

    void begin_session(const AudioConfig& config) override;
    void process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) override;
    [[nodiscard]] RuntimeMetrics metrics() const override;

  private:
    AudioConfig config_{};
    RuntimeMetrics metrics_{};
    std::size_t total_frames_{0};
    std::vector<float> tone_buffer_;

    std::unique_ptr<SineGenerator> tx_generator_;
    std::unique_ptr<Nco> nco_;
    std::unique_ptr<IirLowPass> i_lp_;
    std::unique_ptr<IirLowPass> q_lp_;
    std::unique_ptr<IirLowPass> i_dc_lp_;
    std::unique_ptr<IirLowPass> q_dc_lp_;
    std::unique_ptr<IirLowPass> i_band_lp_;
    std::unique_ptr<IirLowPass> q_band_lp_;
    std::unique_ptr<PhaseTracker> phase_tracker_;
    std::unique_ptr<MotionDetector> detector_;

    double signal_ema_{1e-6};
    double noise_ema_{1e-6};
    double phase_velocity_ema_{0.0};
};

} // namespace sonarlock::core
