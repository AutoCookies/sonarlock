#include "sonarlock/core/dsp_pipeline.hpp"

#include "sonarlock/core/dsp_primitives.hpp"
#include "sonarlock/core/sine_generator.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace sonarlock::core {

BasicDspPipeline::BasicDspPipeline() = default;
BasicDspPipeline::~BasicDspPipeline() = default;

void BasicDspPipeline::begin_session(const AudioConfig& config) {
    config_ = config;
    metrics_ = RuntimeMetrics{};
    metrics_.sample_rate_hz = config.audio.sample_rate_hz;
    metrics_.frames_per_buffer = config.audio.frames_per_buffer;

    total_frames_ = (config.audio.duration_seconds > 0.0)
                       ? static_cast<std::size_t>(config.audio.duration_seconds * config.audio.sample_rate_hz)
                       : static_cast<std::size_t>(config.audio.sample_rate_hz * 3600.0);

    tx_generator_ = std::make_unique<SineGenerator>(config.audio.sample_rate_hz, config.audio.f0_hz);
    nco_ = std::make_unique<Nco>(config.audio.sample_rate_hz, config.audio.f0_hz);
    i_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.lp_cutoff_hz);
    q_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.lp_cutoff_hz);
    i_dc_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.doppler_band_low_hz);
    q_dc_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.doppler_band_low_hz);
    i_band_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.doppler_band_high_hz);
    q_band_lp_ = std::make_unique<IirLowPass>(config.audio.sample_rate_hz, config.dsp.doppler_band_high_hz);
    phase_tracker_ = std::make_unique<PhaseTracker>();
    detector_ = std::make_unique<MotionDetector>(config.detection, std::make_unique<DefaultMotionScorer>());
    calibration_ = std::make_unique<CalibrationController>(config.calibration, config.detection);
    action_policy_ = std::make_unique<DefaultActionPolicy>();
    safety_ = std::make_unique<ActionSafetyController>(config.detection);

    signal_ema_ = 1e-6;
    noise_ema_ = 1e-6;
    phase_velocity_ema_ = 0.0;
    has_prev_input_ = false;
    prev_input_ = 0.0;
    journal_.push("{\"type\":\"session_start\"}");
}

void BasicDspPipeline::process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) {
    if (output.size() != input.size() || !nco_ || !tx_generator_) return;

    tone_buffer_.assign(output.size(), 0.0F);
    tx_generator_->generate(tone_buffer_, total_frames_, frame_offset);
    std::copy(tone_buffer_.begin(), tone_buffer_.end(), output.begin());

    double sum_sq = 0.0;
    double sum = 0.0;
    float peak = 0.0F;

    double bb_sum_sq = 0.0;
    double doppler_sum_sq = 0.0;
    double phase_vel_sum = 0.0;
    double last_unwrapped = 0.0;
    bool has_last = false;

    for (float sample : input) {
        peak = std::max(peak, std::abs(sample));
        sum_sq += static_cast<double>(sample) * sample;
        sum += sample;

        const auto [c, s] = nco_->next();
        const double i = i_lp_->process(static_cast<double>(sample) * c);
        const double q = q_lp_->process(static_cast<double>(sample) * (-s));

        const double mag = std::sqrt(i * i + q * q);
        bb_sum_sq += mag * mag;

        const double i_bp = i_band_lp_->process(i - i_dc_lp_->process(i));
        const double q_bp = q_band_lp_->process(q - q_dc_lp_->process(q));
        const double bp_mag = std::sqrt(i_bp * i_bp + q_bp * q_bp);
        double edge = 0.0;
        if (has_prev_input_) edge = std::abs(static_cast<double>(sample) - prev_input_);
        prev_input_ = sample;
        has_prev_input_ = true;
        doppler_sum_sq += (bp_mag + 0.05 * edge) * (bp_mag + 0.05 * edge);

        const double unwrapped = phase_tracker_->unwrap(i, q);
        if (has_last) {
            const double vel = (unwrapped - last_unwrapped) * config_.audio.sample_rate_hz;
            phase_velocity_ema_ = 0.95 * phase_velocity_ema_ + 0.05 * vel;
            phase_vel_sum += std::abs(phase_velocity_ema_);
        }
        has_last = true;
        last_unwrapped = unwrapped;

        signal_ema_ = 0.995 * signal_ema_ + 0.005 * mag;
        if (bp_mag < 0.01) noise_ema_ = 0.995 * noise_ema_ + 0.005 * mag;
    }

    const double n = static_cast<double>(input.size());
    const double bb = n > 0.0 ? std::sqrt(bb_sum_sq / n) : 0.0;
    const double dop = n > 0.0 ? std::sqrt(doppler_sum_sq / n) : 0.0;

    const bool motion_like = metrics_.latest_event.state == DetectionState::Observing ||
                             metrics_.latest_event.state == DetectionState::Triggered;
    const double alpha = motion_like ? config_.dsp.baseline_motion_alpha : config_.dsp.baseline_alpha;
    metrics_.features.baseline_energy = (1.0 - alpha) * metrics_.features.baseline_energy + alpha * dop;

    metrics_.peak_level = std::max(metrics_.peak_level, peak);
    metrics_.rms_level = n > 0.0 ? static_cast<float>(std::sqrt(sum_sq / n)) : 0.0F;
    metrics_.dc_offset = n > 0.0 ? static_cast<float>(sum / n) : 0.0F;
    metrics_.callbacks += 1;
    metrics_.frames_processed += input.size();

    metrics_.features.baseband_energy = bb;
    metrics_.features.doppler_band_energy = dop;
    metrics_.features.phase_velocity = n > 1.0 ? phase_vel_sum / n : 0.0;
    metrics_.features.snr_estimate = 20.0 * std::log10((signal_ema_ + 1e-6) / (noise_ema_ + 1e-6));
    metrics_.features.relative_motion = std::max(0.0, dop - metrics_.features.baseline_energy);

    const double ts = static_cast<double>(frame_offset + input.size()) / config_.audio.sample_rate_hz;
    DetectionSection det_cfg = config_.detection;
    calibration_->update(ts, metrics_.features.relative_motion, det_cfg);
    detector_->set_detection_config(det_cfg);

    const auto ev = detector_->evaluate(metrics_.features, ts, calibration_->state());
    if (ev.state == DetectionState::Triggered) metrics_.triggered_count += 1;
    metrics_.latest_event = ev;

    const auto req = action_policy_->map(ev, config_.actions.mode);
    metrics_.latest_action = safety_->allow(req, config_.actions.manual_disable, ts) ? req : ActionRequest{};

    std::ostringstream os;
    os << "{\"t\":" << ts << ",\"state\":" << static_cast<int>(ev.state)
       << ",\"cal\":" << static_cast<int>(ev.calibration) << ",\"score\":" << ev.score
       << ",\"rel\":" << metrics_.features.relative_motion << ",\"action\":" << static_cast<int>(metrics_.latest_action.type) << '}';
    journal_.push(os.str());
}

RuntimeMetrics BasicDspPipeline::metrics() const { return metrics_; }

std::string BasicDspPipeline::dump_events_json(std::size_t n) const { return journal_.dump_json_array(n); }

} // namespace sonarlock::core
