// BLE iBeacon transmitter
//
// Uses free bluetooth radio time to transmit an iBeacon compatible
// advertisement, so a fleet of paxcounters can be turned into a remotely
// configurable BLE beacon network.
//
// Note: the ESP32 has only one BLE radio. The beacon transmitter uses the
// Bluedroid BLE stack, while the paxcounter BLE scanner (libpax) talks
// directly to the BT controller via raw HCI. Both cannot run at the same
// time, thus beacon and BLE scanner are mutually exclusive at runtime.

#include "beacon.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

#ifndef BEACON_INTERVAL
#define BEACON_INTERVAL 100 // [milliseconds]
#endif

// iBeacon advertisement data:
// 3 bytes flags AD structure + 27 bytes manufacturer specific AD structure
// (1 length + 1 type + 2 company id + 1 ibeacon type + 1 ibeacon length +
// 16 uuid + 2 major + 2 minor + 1 measured power)
#define IBEACON_MEASURED_POWER ((int8_t)-59) // RSSI @ 1m distance, typical value
#define IBEACON_ADV_DATA_LEN 30

static uint8_t ibeacon_adv_data[IBEACON_ADV_DATA_LEN];
static bool beacon_running = false;

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min = (BEACON_INTERVAL * 1000) / 625,
    .adv_int_max = (BEACON_INTERVAL * 1000) / 625,
    .adv_type = ADV_TYPE_NONCONN_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// build the raw iBeacon advertisement data from current runtime config
static void beacon_build_adv_data(void) {
  uint8_t idx = 0;

  ibeacon_adv_data[idx++] = 0x02; // AD structure length
  ibeacon_adv_data[idx++] = 0x01; // AD type: flags
  ibeacon_adv_data[idx++] = 0x06; // LE general discoverable, BR/EDR not supported

  ibeacon_adv_data[idx++] = 0x1a; // AD structure length (26 bytes follow)
  ibeacon_adv_data[idx++] = 0xff; // AD type: manufacturer specific data
  ibeacon_adv_data[idx++] = 0x4c; // company identifier LSB (Apple)
  ibeacon_adv_data[idx++] = 0x00; // company identifier MSB
  ibeacon_adv_data[idx++] = 0x02; // iBeacon type
  ibeacon_adv_data[idx++] = 0x15; // iBeacon data length (21 bytes follow)

  memcpy(&ibeacon_adv_data[idx], cfg.beaconuuid, sizeof(cfg.beaconuuid));
  idx += sizeof(cfg.beaconuuid);

  ibeacon_adv_data[idx++] = (uint8_t)(cfg.beaconmajor >> 8);
  ibeacon_adv_data[idx++] = (uint8_t)(cfg.beaconmajor & 0xff);
  ibeacon_adv_data[idx++] = (uint8_t)(cfg.beaconminor >> 8);
  ibeacon_adv_data[idx++] = (uint8_t)(cfg.beaconminor & 0xff);
  ibeacon_adv_data[idx++] = (uint8_t)IBEACON_MEASURED_POWER;

  _ASSERT(idx == IBEACON_ADV_DATA_LEN);
}

static void beacon_gap_event_handler(esp_gap_ble_cb_event_t event,
                                     esp_ble_gap_cb_param_t *param) {
  switch (event) {
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    esp_ble_gap_start_advertising(&ble_adv_params);
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
      ESP_LOGE(TAG, "Beacon: failed to start advertising");
    else
      ESP_LOGI(TAG, "Beacon: advertising started");
    break;
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
      ESP_LOGE(TAG, "Beacon: failed to stop advertising");
    else
      ESP_LOGI(TAG, "Beacon: advertising stopped");
    break;
  default:
    break;
  }
}

bool beacon_isrunning(void) { return beacon_running; }

void beacon_start(void) {
  if (beacon_running)
    return;

  if (cfg.blescan) {
    ESP_LOGW(TAG, "Beacon: cannot start, BLE scanner is using the BLE radio. "
                  "Disable BLE scanner first.");
    return;
  }

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Beacon: BT controller init failed");
    return;
  }
  if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
    ESP_LOGE(TAG, "Beacon: BT controller enable failed");
    return;
  }
  if (esp_bluedroid_init() != ESP_OK || esp_bluedroid_enable() != ESP_OK) {
    ESP_LOGE(TAG, "Beacon: Bluedroid stack init failed");
    return;
  }
  if (esp_ble_gap_register_callback(beacon_gap_event_handler) != ESP_OK) {
    ESP_LOGE(TAG, "Beacon: GAP callback registration failed");
    return;
  }

  beacon_build_adv_data();
  esp_ble_gap_config_adv_data_raw(ibeacon_adv_data, sizeof(ibeacon_adv_data));

  beacon_running = true;
  ESP_LOGI(TAG, "Beacon: transmitter started");
}

void beacon_stop(void) {
  if (!beacon_running)
    return;

  esp_ble_gap_stop_advertising();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  beacon_running = false;
  ESP_LOGI(TAG, "Beacon: transmitter stopped");
}

void beacon_update(void) {
  if (!beacon_running)
    return;

  beacon_build_adv_data();
  esp_ble_gap_config_adv_data_raw(ibeacon_adv_data, sizeof(ibeacon_adv_data));
}

void beacon_init(void) {
  ESP_LOGI(TAG, "Beacon: %s", cfg.beacon ? "on" : "off");
  if (cfg.beacon)
    beacon_start();
}
