#include "sonarlock/audio/portaudio_backend.hpp"

#include "sonarlock/core/types.hpp"

#if defined(SONARLOCK_HAS_PORTAUDIO)
#include <portaudio.h>
#endif

#include <algorithm>
#include <vector>

namespace sonarlock::audio {

std::vector<core::AudioDeviceInfo> PortAudioBackend::enumerate_devices() const {
#if defined(SONARLOCK_HAS_PORTAUDIO)
    std::vector<core::AudioDeviceInfo> devices;
    if (Pa_Initialize() != paNoError) {
        return devices;
    }
    const int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info == nullptr) {
            continue;
        }
        devices.push_back(core::AudioDeviceInfo{i, info->name ? info->name : "unknown", info->maxInputChannels,
                                                info->maxOutputChannels, info->defaultSampleRate});
    }
    Pa_Terminate();
    return devices;
#else
    return {};
#endif
}

core::Status PortAudioBackend::run_session(const core::AudioConfig& config, core::IDspPipeline& pipeline,
                                           core::RuntimeMetrics& out_metrics) {
#if defined(SONARLOCK_HAS_PORTAUDIO)
    if (Pa_Initialize() != paNoError) {
        return core::Status::error(core::kErrBackendUnavailable, "failed to initialize PortAudio");
    }

    struct Context {
        core::IDspPipeline* pipeline;
        std::size_t frame_offset;
        std::size_t total_frames;
        core::RuntimeMetrics metrics;
    } ctx{&pipeline, 0, static_cast<std::size_t>(config.dsp.sample_rate_hz * config.dsp.duration_seconds), {}};

    pipeline.begin_session(config);

    PaStreamParameters in_params{};
    PaStreamParameters out_params{};
    in_params.device = Pa_GetDefaultInputDevice();
    out_params.device = Pa_GetDefaultOutputDevice();
    if (in_params.device == paNoDevice || out_params.device == paNoDevice) {
        Pa_Terminate();
        return core::Status::error(core::kErrAudioDeviceUnavailable, "missing default input/output device");
    }

    in_params.channelCount = 1;
    out_params.channelCount = 1;
    in_params.sampleFormat = paFloat32;
    out_params.sampleFormat = paFloat32;
    in_params.suggestedLatency = Pa_GetDeviceInfo(in_params.device)->defaultLowInputLatency;
    out_params.suggestedLatency = Pa_GetDeviceInfo(out_params.device)->defaultLowOutputLatency;

    PaStream* stream = nullptr;
    const auto cb = [](const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                       const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* user_data) {
        auto* ctx = static_cast<Context*>(user_data);
        auto* input = static_cast<const float*>(input_buffer);
        auto* output = static_cast<float*>(output_buffer);

        const std::size_t remaining = ctx->total_frames > ctx->frame_offset ? (ctx->total_frames - ctx->frame_offset) : 0;
        const std::size_t frames = std::min<std::size_t>(frames_per_buffer, remaining);

        std::vector<float> in(frames, 0.0F);
        if (input != nullptr) {
            std::copy(input, input + frames, in.begin());
        }
        std::vector<float> out(frames, 0.0F);
        ctx->pipeline->process(in, out, ctx->frame_offset);

        if (output != nullptr) {
            std::fill(output, output + frames_per_buffer, 0.0F);
            std::copy(out.begin(), out.end(), output);
        }

        ctx->frame_offset += frames;
        return (ctx->frame_offset >= ctx->total_frames) ? paComplete : paContinue;
    };

    const PaError open_err = Pa_OpenStream(&stream, &in_params, &out_params, config.dsp.sample_rate_hz,
                                           static_cast<unsigned long>(config.dsp.frames_per_buffer), paNoFlag, cb, &ctx);
    if (open_err != paNoError) {
        Pa_Terminate();
        return core::Status::error(core::kErrStreamFailure, "failed to open PortAudio stream");
    }

    if (Pa_StartStream(stream) != paNoError) {
        Pa_CloseStream(stream);
        Pa_Terminate();
        return core::Status::error(core::kErrStreamFailure, "failed to start PortAudio stream");
    }

    while (Pa_IsStreamActive(stream) == 1) {
        Pa_Sleep(10);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    out_metrics = pipeline.metrics();
    return core::Status::success();
#else
    (void)config;
    (void)pipeline;
    (void)out_metrics;
    return core::Status::error(core::kErrBackendUnavailable, "PortAudio backend unavailable: dependency not found");
#endif
}

} // namespace sonarlock::audio
