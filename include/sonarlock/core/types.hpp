#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace sonarlock::core {

struct AudioConfig {
    double sample_rate_hz{48000.0};
    std::size_t frames_per_buffer{256};
    double duration_seconds{5.0};
    double tone_frequency_hz{19000.0};
};

enum class BackendKind {
    Real,
    Fake,
};

enum class FakeInputMode {
    Silence,
    ToneWithNoise,
};

enum class SessionState {
    Idle,
    Running,
    Stopped,
    Error,
};

struct AudioDeviceInfo {
    int id{-1};
    std::string name;
    int max_input_channels{0};
    int max_output_channels{0};
    double default_sample_rate{0.0};
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
