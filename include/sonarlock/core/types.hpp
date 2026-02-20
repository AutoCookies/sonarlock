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
enum class CalibrationState { Init, Warmup, Calibrating, Armed };
enum class ActionMode { Soft, Lock, Notify };
enum class ActionType { None, Beep, LockScreen, Notify };

struct AudioSection {
    double sample_rate_hz{48000.0};
    std::size_t frames_per_buffer{256};
    double duration_seconds{5.0}; // 0 => run until stop requested
    double f0_hz{19000.0};
};

struct DspSection {
    double lp_cutoff_hz{500.0};
    double doppler_band_low_hz{20.0};
    double doppler_band_high_hz{200.0};
    double baseline_alpha{0.004};
    double baseline_motion_alpha{0.0004};
};

struct CalibrationSection {
    bool enabled{true};
    double warmup_seconds{2.0};
    double calibrate_seconds{6.0};
    double trigger_k{6.0};
    double release_k{4.0};
    double min_threshold{0.20};
    double max_threshold{0.95};
};

struct DetectionSection {
    double trigger_threshold{0.52};
    double release_threshold{0.38};
    std::uint32_t debounce_ms{300};
    std::uint32_t cooldown_ms{3000};
    std::uint32_t arming_delay_ms{2000};
    std::uint32_t lock_cooldown_ms{30000};
    std::uint32_t max_locks_per_minute{2};
};

struct ActionsSection {
    ActionMode mode{ActionMode::Soft};
    bool manual_disable{false};
};

struct LoggingSection {
    std::string file_path{"sonarlock.log"};
    std::size_t rotate_size_bytes{1024 * 1024};
    std::uint32_t rotate_count{3};
};

struct AppConfig {
    AudioSection audio{};
    DspSection dsp{};
    CalibrationSection calibration{};
    DetectionSection detection{};
    ActionsSection actions{};
    LoggingSection logging{};
    FakeScenario scenario{FakeScenario::Static};
    std::uint32_t seed{7};
    bool daemon_mode{false};
};

using AudioConfig = AppConfig;

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
    double baseline_energy{0.0};
    double relative_motion{0.0};
};

struct MotionEvent {
    DetectionState state{DetectionState::Idle};
    CalibrationState calibration{CalibrationState::Init};
    double score{0.0};
    double confidence{0.0};
    double timestamp_sec{0.0};
};

struct ActionRequest {
    ActionType type{ActionType::None};
    double timestamp_sec{0.0};
    std::string reason;
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
    ActionRequest latest_action{};
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
