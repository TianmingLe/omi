#include <unity.h>

#include "gatt/gatt_mtu_phy.h"
#include "opus_encoder.h"
#include "protocols/ble_audio_protocol.h"

static void test_gatt_mtu_phy_api_compiles()
{
    gatt_mtu_phy_init();
    TEST_ASSERT_TRUE(gatt_mtu_phy_has_peer_addr() == false || gatt_mtu_phy_has_peer_addr() == true);
    TEST_ASSERT_TRUE(gatt_mtu_phy_get_phy_mode_or_unknown() >= 0);
}

static void test_opus_config_constants()
{
    TEST_ASSERT_EQUAL_INT(64000, opus_get_bitrate());
    TEST_ASSERT_EQUAL_INT(5, opus_get_default_complexity());
    TEST_ASSERT_EQUAL_INT(320, opus_get_frame_samples());
    TEST_ASSERT_TRUE(opus_get_output_max_bytes() <= 160);
}

static void test_ble_audio_header_v2_encode_decode()
{
    BleAudioHeaderV2 h{.seq = 65535, .timestamp_ms = 123456};
    uint8_t out[BLE_AUDIO_HEADER_V2_SIZE] = {0};
    ble_audio_header_v2_write(out, h);

    BleAudioHeaderV2 r{.seq = 0, .timestamp_ms = 0};
    ble_audio_header_v2_read(out, &r);

    TEST_ASSERT_EQUAL_UINT16(65535, r.seq);
    TEST_ASSERT_EQUAL_UINT32(123456, r.timestamp_ms);

    BleAudioHeaderV2 h2{.seq = (uint16_t) (h.seq + 1), .timestamp_ms = 123460};
    TEST_ASSERT_EQUAL_UINT16(0, h2.seq);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_gatt_mtu_phy_api_compiles);
    RUN_TEST(test_opus_config_constants);
    RUN_TEST(test_ble_audio_header_v2_encode_decode);
    UNITY_END();
}

void loop() {}
