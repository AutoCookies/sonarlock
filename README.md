<h1>SonarLock</h1>

<div align="center">
  <img src="assets/logo.png" alt="SonarLock Logo" width="180"/>

  

  <p>Acoustic radar security prototype with deterministic fake-mode testing and safety-first action controls.</p>

  [![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/nnkfW83n)
  ![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
  ![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
  ![PortAudio](https://img.shields.io/badge/PortAudio-optional-lightgrey?style=for-the-badge)
</div>

## Quick start (safe mode)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/sonarlock analyze --backend fake --scenario human --duration 3 --action soft
```

## Real run (soft mode)

```bash
./build/sonarlock run --backend real --duration 0 --action soft
```

If real devices/backend are unavailable, SonarLock exits gracefully with clear error codes.

## Enable lock mode (read first)

```bash
./build/sonarlock run --backend real --duration 0 --action lock
```

Safety controls always apply:
- arming delay before first lock
- lock cooldown
- max locks per minute
- manual disable (`--disable-actions`)

## Commands

- `devices`
- `calibrate`
- `run`
- `analyze`
- `dump-events`

Examples:
```bash
./build/sonarlock calibrate --backend fake --scenario static
./build/sonarlock run --backend fake --scenario human --duration 10
./build/sonarlock dump-events --dump-count 100
```

## Config

Use `--config path.json` or default path:
- Linux: `~/.config/sonarlock/config.json`
- Windows: `%APPDATA%\sonarlock\config.json`

CLI flags override config values.

## Packaging guidance

- Linux: install PortAudio optionally, run binary directly; optional systemd unit is documented in `docs/CONFIG.md`.
- Windows: build with Visual Studio, run `sonarlock.exe`; optional Startup folder guidance in `docs/CONFIG.md`.

## Troubleshooting

- If ultrasonic fails, lower `f0` (for example `--f0 17500`).
- Disable OS audio enhancements/noise suppression.
- Keep action mode `soft` while tuning.
