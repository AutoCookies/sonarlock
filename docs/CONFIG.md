# Configuration Reference (JSON)

Example:
```json
{
  "sample_rate_hz": 48000,
  "frames_per_buffer": 256,
  "duration_seconds": 0,
  "f0_hz": 19000,
  "lp_cutoff_hz": 500,
  "doppler_band_low_hz": 20,
  "doppler_band_high_hz": 200,
  "trigger_threshold": 0.52,
  "release_threshold": 0.38,
  "debounce_ms": 300,
  "cooldown_ms": 3000,
  "action_mode": "soft"
}
```

## Defaults

- Audio: 48kHz, 256 frames, 19kHz
- Calibration: warmup 2s, calibrate 6s, enabled
- Detection: debounce 300ms, cooldown 3000ms
- Safety: arming delay 2000ms, lock cooldown 30000ms, max locks/min 2

## Logging

- File logger with size rotation.
- Configure file path, max size, rotation count in code defaults / CLI override path support.

## Deployment notes

### Linux
- Optional systemd unit (document-only): run in soft mode first, then lock mode after validation.

### Windows
- Optional Startup folder: add shortcut to `sonarlock.exe` with desired args.
