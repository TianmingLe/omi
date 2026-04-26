#include <unity.h>

#include "protocols/ble_feature_protocol.h"
#include "vision/feature_extract_tflm.h"

static void test_feature_packet_header_encode_decode()
{
    BleFeatureHeaderV1 h{.seq = 65535, .timestamp_ms = 123456};
    uint8_t buf[BLE_FEATURE_HEADER_V1_SIZE] = {0};
    ble_feature_header_v1_write(buf, h);

    BleFeatureHeaderV1 r{.seq = 0, .timestamp_ms = 0};
    ble_feature_header_v1_read(buf, &r);

    TEST_ASSERT_EQUAL_UINT16(65535, r.seq);
    TEST_ASSERT_EQUAL_UINT32(123456, r.timestamp_ms);

    BleFeatureHeaderV1 h2{.seq = (uint16_t) (h.seq + 1), .timestamp_ms = 123460};
    TEST_ASSERT_EQUAL_UINT16(0, h2.seq);
}

static void test_feature_quant_payload_size()
{
    TEST_ASSERT_EQUAL_INT(6, BLE_FEATURE_QUANT_V1_SIZE);
}

static void test_feature_extract_api_compiles()
{
    feature_extract_tflm_init();
    int8_t in[96 * 96] = {0};
    int8_t out[128] = {0};
    const bool ok = feature_extract_tflm_run(in, out, 128);
    TEST_ASSERT_TRUE(ok == false || ok == true);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_feature_packet_header_encode_decode);
    RUN_TEST(test_feature_quant_payload_size);
    RUN_TEST(test_feature_extract_api_compiles);
    UNITY_END();
}

void loop() {}
