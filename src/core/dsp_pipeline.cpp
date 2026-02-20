#include "sonarlock/core/dsp_pipeline.hpp"

#include "sonarlock/core/sine_generator.hpp"

#include <algorithm>
#include <cmath>

namespace sonarlock::core {

BasicDspPipeline::BasicDspPipeline(double frequency_hz) : frequency_hz_(frequency_hz) {}

BasicDspPipeline::~BasicDspPipeline() = default;

void BasicDspPipeline::begin_session(const AudioConfig& config) {
    config_ = config;
    metrics_ = RuntimeMetrics{};
    metrics_.sample_rate_hz = config.sample_rate_hz;
    metrics_.frames_per_buffer = config.frames_per_buffer;
    total_frames_ = static_cast<std::size_t>(config.duration_seconds * config.sample_rate_hz);
    generator_ = std::make_unique<SineGenerator>(config.sample_rate_hz, frequency_hz_);
}

void BasicDspPipeline::process(std::span<const float> input, std::span<float> output, std::size_t frame_offset) {
    if (output.size() != input.size() || !generator_) {
        return;
    }

    tone_buffer_.assign(output.size(), 0.0F);
    generator_->generate(tone_buffer_, total_frames_, frame_offset);
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = tone_buffer_[i];
    }

    double sum_sq = 0.0;
    double sum = 0.0;
    float peak = 0.0F;
    for (float sample : input) {
        const float mag = std::abs(sample);
        peak = std::max(peak, mag);
        sum_sq += static_cast<double>(sample) * static_cast<double>(sample);
        sum += sample;
    }

    const double n = static_cast<double>(input.size());
    metrics_.peak_level = std::max(metrics_.peak_level, peak);
    metrics_.rms_level = n > 0.0 ? static_cast<float>(std::sqrt(sum_sq / n)) : 0.0F;
    metrics_.dc_offset = n > 0.0 ? static_cast<float>(sum / n) : 0.0F;
    metrics_.callbacks += 1;
    metrics_.frames_processed += input.size();
}

RuntimeMetrics BasicDspPipeline::metrics() const { return metrics_; }

} // namespace sonarlock::core
