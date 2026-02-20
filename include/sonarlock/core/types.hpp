#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sonarlock::core {

enum class BackendKind { Real, Fake };
enum class FakeScenario { Static, Human, Pet, Vibration };
enum class SessionState { Idle, Running, Stopped, Error };
enum class DetectionState { Idle, Observing, Triggered, Cooldown };

struct DspConfig {
    double sample_rate_hz{48000.0};
    std::size_t frames_per_buffer{256};
    double duration_seconds{5.0};
    double f0_hz{19000.0};
    double lp_cutoff_hz{500.0};
    double doppler_band_low_hz{20.0};
    double doppler_band_high_hz{200.0};
};

struct DetectionConfig {
    double trigger_threshold{0.52};
    double release_threshold{0.38};
    std::uint32_t debounce_ms{300};
    std::uint32_t cooldown_ms{3000};
};

struct AudioConfig {
    DspConfig dsp{};
    DetectionConfig detection{};
    FakeScenario scenario{FakeScenario::Static};
    std::uint32_t seed{7};
};

struct AudioDeviceInfo {
    int id{-1};
    std::string name;
    int max_input_channels{0};
    int max_output_channels{0};
    double default_sample_rate{0.0};
};

struct MotionFeatures {
    double baseband_energy{0.0};
    double doppler_band_energy{0.0};
    double phase_velocity{0.0};
    double snr_estimate{0.0};
};

struct MotionEvent {
    DetectionState state{DetectionState::Idle};
    double score{0.0};
    double confidence{0.0};
    double timestamp_sec{0.0};
};

struct RuntimeMetrics {
    double sample_rate_hz{0.0};
    std::size_t frames_per_buffer{0};
    float peak_level{0.0F};
    float rms_level{0.0F};
    float dc_offset{0.0F};
    std::uint64_t xruns{0};
    std::uint64_t callbacks{0};
    std::uint64_t frames_processed{0};

    MotionFeatures features{};
    MotionEvent latest_event{};
    std::uint64_t triggered_count{0};
};

struct Status {
    int code{0};
    std::string message;
    [[nodiscard]] bool ok() const { return code == 0; }
    static Status success() { return {}; }
    static Status error(int c, std::string msg) { return Status{c, std::move(msg)}; }
};

constexpr int kErrInvalidArgument = 2;
constexpr int kErrBackendUnavailable = 3;
constexpr int kErrAudioDeviceUnavailable = 4;
constexpr int kErrStreamFailure = 5;

} // namespace sonarlock::core
