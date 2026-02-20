# Architecture (Phase 2)

Phase 2 keeps the same layering: `app -> (audio, core)` and `audio -> core`; `core` has no OS/audio dependencies.

## Signal flow

```mermaid
flowchart LR
  RX[Mic/RX samples] --> MIX[DownMixerIQ (NCO @ f0)]
  MIX --> LP[IIR Low-pass 500 Hz]
  LP --> FEAT[MotionFeatures\nbaseband energy\ndoppler band energy\nphase velocity\nSNR]
  FEAT --> SCORE[IMotionScorer strategy]
  SCORE --> FSM[DetectionStateMachine\nIDLE->OBSERVING->TRIGGERED->COOLDOWN]
  FSM --> EVT[MotionEvent output]
  TX[TX sine @ f0] --> SPK[Speaker/TX output]
```

## DSP choices

- Numeric type: `double` for DSP internals, `float` for audio sample buffers.
- I/Q coherent demodulation centered at `f0` via continuous-phase NCO.
- Low-pass filter: **first-order IIR**.
  - Chosen for deterministic, stateful streaming behavior, tiny CPU cost, and stable cross-platform performance.
- Doppler band proxy (20-200 Hz): LP-based high-pass (`x - LP20(x)`) followed by LP200.
- Phase tracking: `atan2(Q,I)` + unwrap with ±π continuity correction.

Why baseband I/Q over “FFT at 20k”:
- Doppler motion is low-frequency modulation around carrier; coherent down-mixing extracts that directly.
- Lower computational cost and better frame-to-frame temporal continuity than repeated FFT bins at ultrasonic carrier.

## Defaults

- sample rate: 48000 Hz
- frames: 256
- duration: 5 s
- f0: 19000 Hz
- lp cutoff: 500 Hz
- doppler band: 20-200 Hz
- trigger threshold: 0.52
- release threshold: 0.38
- debounce: 300 ms
- cooldown: 3000 ms

## Extension boundary for Phase 3+

- Swap/augment `IMotionScorer` strategies without backend changes.
- Add richer filtering/policies while preserving existing `IDspPipeline` and `IAudioBackend` contracts.
