#include "sonarlock/audio/audio_factory.hpp"

#include "sonarlock/audio/fake_audio_backend.hpp"
#include "sonarlock/audio/portaudio_backend.hpp"

namespace sonarlock::audio {

std::unique_ptr<core::IAudioBackend> make_backend(core::BackendKind kind, core::FakeInputMode fake_mode,
                                                  std::uint32_t seed) {
    if (kind == core::BackendKind::Real) {
        return std::make_unique<PortAudioBackend>();
    }
    return std::make_unique<FakeAudioBackend>(fake_mode, seed);
}

} // namespace sonarlock::audio
