#pragma once

#include <stddef.h>
#include <stdint.h>

class BLECharacteristic;

void ble_audio_stream_init(BLECharacteristic *audioChar);
void ble_audio_stream_send(const uint8_t *opus_payload, size_t len, uint32_t capture_ts_ms);
void ble_audio_stream_note_non_audio_tx(size_t bytes);
bool ble_audio_stream_is_congested();
