#pragma once

#include "sonarlock/core/action_policy.hpp"
#include "sonarlock/core/calibration.hpp"
#include "sonarlock/core/event_journal.hpp"
#include "sonarlock/core/motion_detection.hpp"
#include "sonarlock/core/types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
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
    [[nodiscard]] std::string dump_events_json(std::size_t n) const;

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
    std::unique_ptr<CalibrationController> calibration_;
    std::unique_ptr<IActionPolicy> action_policy_;
    std::unique_ptr<ActionSafetyController> safety_;
    EventJournal journal_{200};

    double signal_ema_{1e-6};
    double noise_ema_{1e-6};
    double phase_velocity_ema_{0.0};
    double prev_input_{0.0};
    bool has_prev_input_{false};
};

} // namespace sonarlock::core
