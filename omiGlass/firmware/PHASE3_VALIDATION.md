# Phase 3 Validation (Vision + TFLM + BLE Feature)

## Commits

- Baseline (pre-Phase3): `975d24334`
- Current (Phase3 HEAD): `d9beec8eb`

## Memory Gate: `.dram0.bss` Delta

### Commands

```bash
platformio run -e seeed_xiao_esp32s3 -t size
platformio run -e seeed_xiao_esp32s3 -t size --verbose
```

Section extraction (for `.dram0.bss`):

```bash
/root/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-size -A .pio/build/seeed_xiao_esp32s3/firmware.elf
```

### Results

| Commit | `.dram0.bss` (bytes) |
|---|---:|
| `975d24334` | 87920 |
| `d9beec8eb` | 88264 |

Delta: `+344` bytes (gate: `≤ 30720` bytes)

## TFLM Invoke Telemetry

The firmware logs a timing + arena usage line for each `Invoke()`:

```
I (VISION) TFLM_INVOKE: duration_ms=XX, arena_used=YYYY
```

Note: inference latency must be verified on hardware via serial logs (target: `≤ 20ms`).

## Build & Test

### Firmware build

```bash
platformio run -e seeed_xiao_esp32s3
```

### Test build gate (no upload, no execution)

```bash
platformio test -e seeed_xiao_esp32s3_test --without-uploading --without-testing
```

