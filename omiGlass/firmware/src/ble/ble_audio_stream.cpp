#include "ble_audio_stream.h"

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <esp_log.h>

#include "config.h"
#include "protocols/ble_audio_protocol.h"

static BLECharacteristic *s_audio_char = nullptr;
static uint16_t s_seq = 0;
static uint8_t s_packet[BLE_AUDIO_HEADER_V2_SIZE + OPUS_OUTPUT_MAX_BYTES];
static uint32_t s_window_start_ms = 0;
static uint32_t s_tx_bytes_1s = 0;
static uint32_t s_last_capture_ts_ms = 0;
static uint32_t s_last_encode_to_send_ms = 0;

void ble_audio_stream_init(BLECharacteristic *audioChar)
{
    s_audio_char = audioChar;
    s_seq = 0;
    s_window_start_ms = millis();
    s_tx_bytes_1s = 0;
    s_last_capture_ts_ms = 0;
    s_last_encode_to_send_ms = 0;
}

static void window_tick(uint32_t now_ms)
{
    if (now_ms - s_window_start_ms < 1000) {
        return;
    }
    const bool congested = (s_tx_bytes_1s * 8U) > 400000U;
    ESP_LOGD(
        "BLE_AUDIO",
        "BLE_AUDIO_BW: tx_bytes_1s=%lu congested=%d last_capture_ts=%lu encode_to_send_ms=%lu",
        (unsigned long) s_tx_bytes_1s,
        congested ? 1 : 0,
        (unsigned long) s_last_capture_ts_ms,
        (unsigned long) s_last_encode_to_send_ms
    );
    s_window_start_ms = now_ms;
    s_tx_bytes_1s = 0;
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

    const uint32_t now_ms = millis();
    window_tick(now_ms);
    s_tx_bytes_1s += (uint32_t) (BLE_AUDIO_HEADER_V2_SIZE + len);
    s_last_capture_ts_ms = capture_ts_ms;
    s_last_encode_to_send_ms = (capture_ts_ms == 0) ? 0 : (now_ms - capture_ts_ms);

    s_audio_char->setValue(s_packet, BLE_AUDIO_HEADER_V2_SIZE + len);
    s_audio_char->notify();
    s_seq++;
}

void ble_audio_stream_note_non_audio_tx(size_t bytes)
{
    const uint32_t now_ms = millis();
    window_tick(now_ms);
    s_tx_bytes_1s += (uint32_t) bytes;
}

bool ble_audio_stream_is_congested()
{
    window_tick(millis());
    return (s_tx_bytes_1s * 8U) > 400000U;
}
