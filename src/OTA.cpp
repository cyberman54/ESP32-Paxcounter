#include "OTA.h"

const BintrayClient bintray(BINTRAY_USER, BINTRAY_REPO, BINTRAY_PACKAGE);

bool Wifi_Connected = false;

static esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    Wifi_Connected = true;
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Wifi_Connected = false;
    break;
  default:
    break;
  }
}

void ota_wifi_init(void) {

  // initialize the tcp stack
  tcpip_adapter_init();

  // initialize the wifi event handler
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t sta_config = {};

  strcpy((char *)sta_config.sta.ssid, WIFI_SSID);
  strcpy((char *)sta_config.sta.password, WIFI_PASS);
  sta_config.sta.bssid_set = false;

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  // print the local IP address
  tcpip_adapter_ip_info_t ip_info;
  ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
  ESP_LOGI(TAG, "IP %s", ip4addr_ntoa(&ip_info.ip));

}

void start_ota_update() {
  ESP_LOGI(TAG, "Stopping Wifi task on core 0");
  vTaskDelete(WifiLoopTask);

  ESP_LOGI(TAG, "Stopping LORA task on core 1");
  vTaskDelete(LoraTask);

  ESP_LOGI(TAG, "Connecting to %s", WIFI_SSID);
  ota_wifi_init();
  checkFirmwareUpdates();
  ESP.restart(); // reached if update was not successful
}