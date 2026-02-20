# Architecture (Phase 3 / v1.0.0)

```mermaid
flowchart LR
  RX[RX audio] --> MIX[DownMixer I/Q]
  MIX --> LP[LP + Doppler band]
  LP --> FEAT[Features + baseline cancellation]
  FEAT --> CAL[Calibration FSM + AutoTuner]
  CAL --> DET[Detector FSM]
  DET --> POL[ActionPolicy]
  POL --> SAFE[Safety controller]
  SAFE --> ACT[Platform executor]
  DET --> JRN[EventJournal ring buffer]
```

- `core/` is dependency-free and owns DSP, calibration, detection, safety, and event journal.
- `audio/` provides fake + optional real backends.
- `platform/` executes `ActionRequest` (Linux command chain / Windows LockWorkStation).
- `app/` handles CLI, config loading, logging, lifecycle, and shutdown signals.

Filter choice remains first-order IIR for deterministic low-latency streaming with persistent state.
