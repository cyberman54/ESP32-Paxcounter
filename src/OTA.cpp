#include "OTA.h"

const BintrayClient bintray(BINTRAY_USER, BINTRAY_REPO, BINTRAY_PACKAGE);

bool Wifi_Connected = false;

esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    ESP_LOGI(TAG, "Event STA_START");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    Wifi_Connected = true;
    ESP_LOGI(TAG, "Event STA_GOT_IP");
    // print the local IP address
    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
    ESP_LOGI(TAG, "IP %s", ip4addr_ntoa(&ip_info.ip));
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Wifi_Connected = false;
    ESP_LOGI(TAG, "Event STA_DISCONNECTED");
    break;
  default:
    break;
  }
}

void ota_wifi_init(void) {

  tcpip_adapter_if_t tcpip_if = TCPIP_ADAPTER_IF_STA;

  // initialize the tcp stack
  // nvs_flash_init();
  tcpip_adapter_init();
  tcpip_adapter_set_hostname(tcpip_if, PROGNAME);
  tcpip_adapter_dhcpc_start(tcpip_if);

  // initialize the wifi event handler
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  // switch off monitor more
  ESP_ERROR_CHECK(
      esp_wifi_set_promiscuous(false)); // now switch on monitor mode
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL));

  wifi_sta_config_t cfg;
  strcpy((char *)cfg.ssid, WIFI_SSID);
  strcpy((char *)cfg.password, WIFI_PASS);
  cfg.bssid_set = false;

  wifi_config_t sta_cfg;
  sta_cfg.sta = cfg;

  wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
  ESP_ERROR_CHECK(
      esp_wifi_set_storage(WIFI_STORAGE_RAM)); // we don't need NVRAM
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void start_ota_update() {
  ESP_LOGI(TAG, "Stopping Wifi task on core 0");
  vTaskDelete(WifiLoopTask);

  ESP_LOGI(TAG, "Stopping LORA task on core 1");
  vTaskDelete(LoraTask);

  ESP_LOGI(TAG, "Connecting to %s", WIFI_SSID);
  ota_wifi_init();
  delay(2000);
  delay(2000);
  checkFirmwareUpdates();
  ESP.restart(); // reached if update was not successful
}