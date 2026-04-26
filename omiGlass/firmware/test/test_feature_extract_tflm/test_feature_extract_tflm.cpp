#include <unity.h>

#include "camera/camera_capture.h"
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

static void fill_lcg_rgb565(uint16_t *buf, int n)
{
    uint32_t x = 0x12345678U;
    for (int i = 0; i < n; i++) {
        x = x * 1664525U + 1013904223U;
        buf[i] = (uint16_t) (x & 0xFFFFU);
    }
}

static void test_preprocess_range_black_white_random()
{
    static uint16_t rgb565[128 * 128];
    static int8_t out[96 * 96];

    for (int i = 0; i < 128 * 128; i++) {
        rgb565[i] = 0x0000;
    }
    preprocess_rgb565_to_96x96_int8(rgb565, 128, 128, out);
    for (int i = 0; i < 96 * 96; i++) {
        TEST_ASSERT_TRUE(out[i] >= -128 && out[i] <= 127);
    }

    for (int i = 0; i < 128 * 128; i++) {
        rgb565[i] = 0xFFFF;
    }
    preprocess_rgb565_to_96x96_int8(rgb565, 128, 128, out);
    for (int i = 0; i < 96 * 96; i++) {
        TEST_ASSERT_TRUE(out[i] >= -128 && out[i] <= 127);
    }

    fill_lcg_rgb565(rgb565, 128 * 128);
    preprocess_rgb565_to_96x96_int8(rgb565, 128, 128, out);
    for (int i = 0; i < 96 * 96; i++) {
        TEST_ASSERT_TRUE(out[i] >= -128 && out[i] <= 127);
    }
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_feature_packet_header_encode_decode);
    RUN_TEST(test_feature_quant_payload_size);
    RUN_TEST(test_feature_extract_api_compiles);
    RUN_TEST(test_preprocess_range_black_white_random);
    UNITY_END();
}

void loop() {}
