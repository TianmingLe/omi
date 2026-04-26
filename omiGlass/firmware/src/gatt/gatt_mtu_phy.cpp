#include "gatt_mtu_phy.h"

#include <BLEDevice.h>
#include <esp_gap_ble_api.h>
#include <esp_log.h>

static uint8_t s_peer_bda[6];
static bool s_has_peer = false;
static int s_phy_mode = 0;

static void gatt_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    (void) gatts_if;
    if (event == ESP_GATTS_CONNECT_EVT) {
        gatt_mtu_phy_on_connected(param->connect.remote_bda);
    }
}

static void gap_evt_handler_impl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT) {
        const auto status = param->phy_update.status;
        const auto tx_phy = param->phy_update.tx_phy;
        const auto rx_phy = param->phy_update.rx_phy;

        if (status == ESP_BT_STATUS_SUCCESS) {
            if (tx_phy == ESP_BLE_GAP_PHY_2M || rx_phy == ESP_BLE_GAP_PHY_2M) {
                s_phy_mode = 2;
            } else if (tx_phy == ESP_BLE_GAP_PHY_1M || rx_phy == ESP_BLE_GAP_PHY_1M) {
                s_phy_mode = 1;
            } else {
                s_phy_mode = 0;
            }
        } else {
            s_phy_mode = 0;
        }

        ESP_LOGI("BLE_GATT", "BLE_PHY: request=2M result=%d status=%d", s_phy_mode, (int) status);
    }
}

void gatt_mtu_phy_init()
{
    const auto err = BLEDevice::setMTU(251);
    ESP_LOGI("BLE_GATT", "BLE_MTU: requested=251 result=%d", (int) err);

    BLEDevice::setCustomGattsHandler(gatt_event_handler);
    BLEDevice::setCustomGapHandler(gap_evt_handler_impl);
}

void gatt_mtu_phy_on_connected(const uint8_t peer_bda[6])
{
    for (int i = 0; i < 6; i++) {
        s_peer_bda[i] = peer_bda[i];
    }
    s_has_peer = true;
    s_phy_mode = 0;

    ESP_LOGI("BLE_GATT", "BLE_MTU: requested=251 actual=%d", (int) BLEDevice::getMTU());

    const auto err = esp_ble_gap_set_prefered_phy(
        s_peer_bda,
        0,
        ESP_BLE_GAP_PHY_2M_PREF_MASK,
        ESP_BLE_GAP_PHY_2M_PREF_MASK,
        ESP_BLE_GAP_PHY_OPTIONS_NO_PREF
    );
    ESP_LOGI("BLE_GATT", "BLE_PHY: request=2M status=%d", (int) err);
}

int gatt_mtu_phy_get_mtu()
{
    return (int) BLEDevice::getMTU();
}

int gatt_mtu_phy_get_phy_mode_or_unknown()
{
    return s_phy_mode;
}

bool gatt_mtu_phy_has_peer_addr()
{
    return s_has_peer;
}
