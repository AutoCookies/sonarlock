# Architecture (Phase 1)

## Module boundaries

- **core/**
  - Pure logic and contracts (`IAudioBackend`, `IDspPipeline`, `SessionController`, `SineGenerator`).
  - No OS/audio library dependency.
- **audio/**
  - Backend implementations.
  - `FakeAudioBackend` for deterministic CI-safe tests.
  - `PortAudioBackend` for best-effort device enumeration + stream run.
- **app/**
  - CLI argument parsing and runtime orchestration.

Dependency direction is one-way toward `core`: backend and application logic both consume core contracts.

## Key interfaces

- `IAudioBackend`
  - `enumerate_devices()`
  - `run_session(config, pipeline, out_metrics)`
- `IDspPipeline`
  - `begin_session(config)`
  - `process(input, output, frame_offset)`
  - `metrics()`

## Session lifecycle state

`SessionController` enforces explicit state transitions:

`Idle -> Running -> Stopped|Error`

## DSP pipeline (Phase 1)

`BasicDspPipeline`:
- Generates TX sine tone with continuous phase.
- Applies fade-in/out envelope (20 ms) to reduce clicks.
- Computes RX metrics from input stream:
  - RMS
  - Peak
  - DC offset

Hooks for later phases are intentionally kept as extension points in `IDspPipeline` and `BasicDspPipeline` internals.

## Phase 2 extension plan

Future changes should extend `IDspPipeline` implementation only (not backend contracts):
- I/Q down-mix stage
- Band-pass / low-pass filtering chain
- Decision state machine for threat/gesture semantics

This keeps hardware backends stable while evolving DSP and decision logic.
