# Changelog

## v0.1.0-phase1
- Established C++20/CMake target-based architecture with `core`, `audio`, and `app` modules.
- Added `IAudioBackend` and `IDspPipeline` contracts with explicit session lifecycle control.
- Implemented `BasicDspPipeline` with 19 kHz sine generation, phase continuity, fade envelope, and RMS/peak/DC metrics.
- Implemented `FakeAudioBackend` for deterministic hardware-free testing.
- Implemented optional `PortAudioBackend` (best-effort enumeration + run), gracefully unavailable when dependency is missing.
- Added CLI commands: `devices`, `run` with runtime metrics logging.
- Added CI workflow for Linux and Windows configure/build/test.
- Added deterministic unit tests for DSP, fake backend session behavior, and CLI parsing.
