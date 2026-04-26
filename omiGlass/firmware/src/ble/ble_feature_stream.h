#pragma once

#include <stddef.h>
#include <stdint.h>

class BLECharacteristic;

void ble_feature_stream_init(BLECharacteristic *dataChar);
void ble_feature_stream_send(const int8_t *feat_128, uint32_t capture_ts_ms);
uint16_t ble_feature_stream_last_seq();

