# Safety Model

## Mandatory failsafes

- **Arming delay**: no actions immediately after enable.
- **Cooldown after lock**: default 30s.
- **Rate limit**: default max 2 locks/min.
- **Manual disable**: `--disable-actions` blocks all actions for session.
- **Calibration gate**: detector actions only when calibration reaches ARMED.

## Recovery steps

1. Stop process with `Ctrl+C`.
2. Relaunch in safe mode:
   - `--action soft --disable-actions`
3. Re-run calibration:
   - `sonarlock calibrate --backend fake --scenario static`
4. Tune trigger/release thresholds before enabling lock mode.

## Anti-lock-loop guarantee

`ActionSafetyController` enforces cooldown + per-minute caps independent of detector output.
