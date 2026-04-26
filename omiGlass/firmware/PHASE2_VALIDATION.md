# Phase 2 BLE Audio - Validation Summary

## Scope

- MTU request: 251
- PHY: prefer 2M, fallback 1M
- Opus: 16kHz/mono, 20ms, 64kbps CBR, complexity=5 (battery<20% => 3)
- Packet V2 (codec id 22): `[seq(2B) + capture_timestamp_ms(4B) + payload]`
- Throttling: when rolling 1s bandwidth `tx_bytes_1s*8 > 400kbps`, pause JPEG chunk notifies to protect audio

## Build & Test

- Firmware build: `platformio run -e seeed_xiao_esp32s3` => SUCCESS
- Unit test build: `platformio test -e seeed_xiao_esp32s3_test --without-uploading --without-testing` => PASS (compile-only in CI)

## Memory gate (`.dram0.bss`)

- BASE commit: `c6e0f61c951f28fa5e0ed9840db624f0815d8f1e`
  - `.dram0.bss = 87312`
- CUR commit: `3b18c9e8a08b3f2fc5da56d21ba3af42446acfc6`
  - `.dram0.bss = 87920`
- DELTA: `+608 bytes` (target ≤ 8192)

