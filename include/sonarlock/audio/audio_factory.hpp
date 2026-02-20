#pragma once

#include "sonarlock/core/audio_backend.hpp"
#include "sonarlock/core/types.hpp"

#include <memory>

namespace sonarlock::audio {

std::unique_ptr<core::IAudioBackend> make_backend(core::BackendKind kind, core::FakeInputMode fake_mode,
                                                  std::uint32_t seed = 7);

} // namespace sonarlock::audio
