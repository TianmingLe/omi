#pragma once

#include <stdint.h>

static constexpr int BLE_AUDIO_HEADER_V2_SIZE = 6;

struct BleAudioHeaderV2 {
    uint16_t seq;
    uint32_t timestamp_ms;
};

inline void ble_audio_header_v2_write(uint8_t *dst, const BleAudioHeaderV2 &h)
{
    dst[0] = (uint8_t) (h.seq & 0xFF);
    dst[1] = (uint8_t) ((h.seq >> 8) & 0xFF);
    dst[2] = (uint8_t) (h.timestamp_ms & 0xFF);
    dst[3] = (uint8_t) ((h.timestamp_ms >> 8) & 0xFF);
    dst[4] = (uint8_t) ((h.timestamp_ms >> 16) & 0xFF);
    dst[5] = (uint8_t) ((h.timestamp_ms >> 24) & 0xFF);
}

inline void ble_audio_header_v2_read(const uint8_t *src, BleAudioHeaderV2 *out)
{
    out->seq = (uint16_t) (src[0] | (src[1] << 8));
    out->timestamp_ms = (uint32_t) (src[2] | (src[3] << 8) | (src[4] << 16) | (src[5] << 24));
}

