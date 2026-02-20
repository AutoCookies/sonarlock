# Changelog

## v1.0.0
- Added calibration state machine (INIT/WARMUP/CALIBRATING/ARMED) and AutoTuner with clamped robust threshold tuning.
- Added baseline clutter cancellation and motion-relative scoring path.
- Added safety-grade action pipeline: ActionPolicy + ActionRequest + anti-lock-loop controls (arming delay, cooldown, lock-rate cap, manual disable).
- Added core event journal ring buffer with JSON dump output.
- Added platform action executors:
  - Windows: `LockWorkStation()`.
  - Linux: fallback lock command chain.
- Added background-capable CLI behavior (`run --duration 0`, SIGINT clean stop), commands `calibrate` and `dump-events`.
- Added config file loading (`--config` + default home path), file logging with rotation, and v1 safety/usage docs.
- Expanded deterministic CI-safe tests for calibration, baseline freeze, anti-lock-loop, platform execution path abstraction, and integration behavior.
