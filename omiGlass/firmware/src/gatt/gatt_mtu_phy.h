#pragma once

#include <stdbool.h>
#include <stdint.h>

void gatt_mtu_phy_init();
void gatt_mtu_phy_on_connected(const uint8_t peer_bda[6]);
int gatt_mtu_phy_get_mtu();
int gatt_mtu_phy_get_phy_mode_or_unknown();
bool gatt_mtu_phy_has_peer_addr();

