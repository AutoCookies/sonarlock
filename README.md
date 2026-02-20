# SonarLock

Phase 2 adds coherent I/Q baseband processing, motion features, and a detection state machine.

## Build

### Linux
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## CLI

```bash
./build/sonarlock devices --backend fake
./build/sonarlock analyze --backend fake --scenario human
./build/sonarlock analyze --backend fake --scenario static
./build/sonarlock analyze --backend fake --scenario pet
./build/sonarlock analyze --backend fake --scenario vibration
./build/sonarlock analyze --backend fake --scenario human --csv out.csv
```

## CSV output

Columns:
`timestamp,state,score,confidence,baseband_energy,doppler_energy,phase_velocity,snr`

## Optional real backend

`--backend real` uses PortAudio if installed; otherwise the app exits gracefully with clear error codes.
