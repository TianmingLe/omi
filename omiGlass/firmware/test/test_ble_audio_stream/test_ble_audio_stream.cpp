#include <unity.h>

#include "gatt/gatt_mtu_phy.h"

static void test_gatt_mtu_phy_api_compiles()
{
    gatt_mtu_phy_init();
    TEST_ASSERT_TRUE(gatt_mtu_phy_has_peer_addr() == false || gatt_mtu_phy_has_peer_addr() == true);
    TEST_ASSERT_TRUE(gatt_mtu_phy_get_phy_mode_or_unknown() >= 0);
}

void setup()
{
    UNITY_BEGIN();
    test_gatt_mtu_phy_api_compiles();
    UNITY_END();
}

void loop() {}

