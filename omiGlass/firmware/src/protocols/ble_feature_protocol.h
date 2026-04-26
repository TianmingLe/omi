#pragma once

#include <stdint.h>

#ifndef FEATURE_CODEC_ID
#define FEATURE_CODEC_ID 33
#endif

static constexpr int BLE_FEATURE_HEADER_V1_SIZE = 6;
static constexpr int BLE_FEATURE_VECTOR_SIZE = 128;
static constexpr int BLE_FEATURE_PACKET_V1_SIZE = BLE_FEATURE_HEADER_V1_SIZE + BLE_FEATURE_VECTOR_SIZE;
static constexpr int BLE_FEATURE_QUANT_V1_SIZE = 6;

struct BleFeatureHeaderV1 {
    uint16_t seq;
    uint32_t timestamp_ms;
};

inline void ble_feature_header_v1_write(uint8_t *dst, const BleFeatureHeaderV1 &h)
{
    dst[0] = (uint8_t) (h.seq & 0xFF);
    dst[1] = (uint8_t) ((h.seq >> 8) & 0xFF);
    dst[2] = (uint8_t) (h.timestamp_ms & 0xFF);
    dst[3] = (uint8_t) ((h.timestamp_ms >> 8) & 0xFF);
    dst[4] = (uint8_t) ((h.timestamp_ms >> 16) & 0xFF);
    dst[5] = (uint8_t) ((h.timestamp_ms >> 24) & 0xFF);
}

inline void ble_feature_header_v1_read(const uint8_t *src, BleFeatureHeaderV1 *out)
{
    out->seq = (uint16_t) (src[0] | (src[1] << 8));
    out->timestamp_ms = (uint32_t) (src[2] | (src[3] << 8) | (src[4] << 16) | (src[5] << 24));
}

