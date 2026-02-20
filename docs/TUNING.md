# Tuning Guide

## Fake scenarios

Use `--backend fake` and scenario sweeps:
- `--scenario static`
- `--scenario human`
- `--scenario pet`
- `--scenario vibration`

## Metrics to watch

- `doppler_band_energy`: strongest discriminator for real motion.
- `phase_velocity`: helps reject static reflections.
- `snr_estimate`: stabilizes confidence.
- `score/confidence/state`: final detector behavior.

## Practical tuning loop

1. Start with defaults.
2. Run static + vibration; ensure no `TRIGGERED`.
3. Run human; ensure at least one `TRIGGERED` within ~0.5-1.5 s.
4. If false positives occur, raise `--trigger-th` or `--debounce-ms`.
5. If misses occur, lower `--trigger-th` slightly (keep `release-th < trigger-th`).

## Example

```bash
./build/sonarlock analyze --backend fake --scenario human --csv human.csv
./build/sonarlock analyze --backend fake --scenario static --csv static.csv
```
