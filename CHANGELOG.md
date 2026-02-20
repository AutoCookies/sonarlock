# Changelog

## v0.2.0-phase2
- Added coherent I/Q down-mix pipeline with persistent NCO phase.
- Added deterministic first-order IIR low-pass stages for baseband and Doppler-band proxy extraction.
- Added phase tracking with unwrap, motion features (baseband energy, doppler band energy, phase velocity, SNR), and motion scoring strategy.
- Added detection state machine: IDLE -> OBSERVING -> TRIGGERED -> COOLDOWN.
- Added `analyze` CLI mode with DSP/detector configuration flags and optional CSV output.
- Upgraded fake backend with deterministic scenarios: static, human, pet, vibration.
- Expanded deterministic test harness with DSP, detector, integration, and CLI parsing coverage.
