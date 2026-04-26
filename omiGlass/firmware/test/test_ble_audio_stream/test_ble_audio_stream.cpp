#include <unity.h>

#include "gatt/gatt_mtu_phy.h"
#include "opus_encoder.h"

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

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_gatt_mtu_phy_api_compiles);
    RUN_TEST(test_opus_config_constants);
    UNITY_END();
}

void loop() {}
