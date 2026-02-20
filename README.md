# SonarLock v0.1.0-phase1

Phase 1 provides a production-ready foundation for an Acoustic Radar Security prototype with clean module boundaries, deterministic testability, and a loopback session skeleton.

## Build

### Linux
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows (PowerShell)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## CLI usage

### List devices
```bash
./build/sonarlock devices --backend fake
./build/sonarlock devices --backend real
```

### Run loopback session (fake backend)
```bash
./build/sonarlock run --backend fake --duration 5 --freq 19000 --samplerate 48000 --frames 256 --fake-input tone-noise
```

### Run loopback session (real backend, PortAudio installed)
```bash
./build/sonarlock run --backend real --duration 5 --freq 19000 --samplerate 48000 --frames 256
```

If PortAudio is unavailable, the real backend exits gracefully with a clear error code/message.

## Notes
- Default settings: 48 kHz, 256 frames, 5 seconds, 19 kHz tone.
- `core/` is dependency-free business logic.
- `audio/` implements fake + optional real backend.
- `app/` provides CLI wiring.
