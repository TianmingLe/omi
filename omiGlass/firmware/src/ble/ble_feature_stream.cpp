#include "ble_feature_stream.h"

#include <BLECharacteristic.h>

#include "protocols/ble_feature_protocol.h"

static BLECharacteristic *s_data_char = nullptr;
static uint16_t s_seq = 0;
static uint8_t s_packet[BLE_FEATURE_PACKET_V1_SIZE];

void ble_feature_stream_init(BLECharacteristic *dataChar)
{
    s_data_char = dataChar;
    s_seq = 0;
}

void ble_feature_stream_send(const int8_t *feat_128, uint32_t capture_ts_ms)
{
    if (s_data_char == nullptr) {
        return;
    }

    const BleFeatureHeaderV1 header{.seq = s_seq, .timestamp_ms = capture_ts_ms};
    ble_feature_header_v1_write(s_packet, header);

    for (int i = 0; i < BLE_FEATURE_VECTOR_SIZE; i++) {
        s_packet[BLE_FEATURE_HEADER_V1_SIZE + i] = (uint8_t) feat_128[i];
    }

    s_data_char->setValue(s_packet, BLE_FEATURE_PACKET_V1_SIZE);
    s_data_char->notify();
    s_seq++;
}

uint16_t ble_feature_stream_last_seq()
{
    return (uint16_t) (s_seq - 1);
}

