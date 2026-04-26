#include "ble_audio_stream.h"

#include <BLECharacteristic.h>

#include "config.h"
#include "protocols/ble_audio_protocol.h"

static BLECharacteristic *s_audio_char = nullptr;
static uint16_t s_seq = 0;
static uint8_t s_packet[BLE_AUDIO_HEADER_V2_SIZE + OPUS_OUTPUT_MAX_BYTES];

void ble_audio_stream_init(BLECharacteristic *audioChar)
{
    s_audio_char = audioChar;
    s_seq = 0;
}

void ble_audio_stream_send(const uint8_t *opus_payload, size_t len, uint32_t capture_ts_ms)
{
    if (s_audio_char == nullptr) {
        return;
    }
    if (len == 0 || len > OPUS_OUTPUT_MAX_BYTES) {
        return;
    }

    const BleAudioHeaderV2 header{.seq = s_seq, .timestamp_ms = capture_ts_ms};
    ble_audio_header_v2_write(s_packet, header);
    for (size_t i = 0; i < len; i++) {
        s_packet[BLE_AUDIO_HEADER_V2_SIZE + i] = opus_payload[i];
    }

    s_audio_char->setValue(s_packet, BLE_AUDIO_HEADER_V2_SIZE + len);
    s_audio_char->notify();
    s_seq++;
}

