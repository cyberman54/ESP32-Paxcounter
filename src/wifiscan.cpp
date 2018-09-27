// Basic Config
#include "globals.h"
#include "wifiscan.h"

// Local logging tag
static const char TAG[] = "wifi";

static wifi_country_t wifi_country = {WIFI_MY_COUNTRY, WIFI_CHANNEL_MIN,
                                      WIFI_CHANNEL_MAX, 100,
                                      WIFI_COUNTRY_POLICY_MANUAL};

// using IRAM_:ATTR here to speed up callback function
IRAM_ATTR void wifi_sniffer_packet_handler(void *buff,
                                           wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt =
      (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  if ((cfg.rssilimit) &&
      (ppkt->rx_ctrl.rssi < cfg.rssilimit)) // rssi is negative value
    ESP_LOGD(TAG, "WiFi RSSI %d -> ignoring (limit: %d)", ppkt->rx_ctrl.rssi,
             cfg.rssilimit);
  else // count seen MAC
    mac_add((uint8_t *)hdr->addr2, ppkt->rx_ctrl.rssi, MAC_SNIFF_WIFI);
}

void wifi_sniffer_init(void) {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.nvs_enable = 0; // we don't need any wifi settings from NVRAM
  wifi_promiscuous_filter_t filter = {
      .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT}; // we need only MGMT frames

  // esp_event_loop_init(NULL, NULL);
  // ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // configure Wifi with cfg
  ESP_ERROR_CHECK(
      esp_wifi_set_country(&wifi_country)); // set locales for RF and channels
  ESP_ERROR_CHECK(
      esp_wifi_set_storage(WIFI_STORAGE_RAM)); // we don't need NVRAM
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(
      esp_wifi_set_promiscuous_filter(&filter)); // set MAC frame filter
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); // now switch on monitor mode
}

// IRQ Handler
void ChannelSwitchIRQ() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // unblock wifi channel rotation task
  xSemaphoreGiveFromISR(xWifiChannelSwitchSemaphore, &xHigherPriorityTaskWoken);
}

// Wifi channel rotation task
void switchWifiChannel(void * parameter) {
  while (1) {
    // task in block state to wait for channel switch timer interrupt event
    xSemaphoreTake(xWifiChannelSwitchSemaphore, portMAX_DELAY);
    // rotates variable channel 1..WIFI_CHANNEL_MAX
    channel = (channel % WIFI_CHANNEL_MAX) + 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    ESP_LOGD(TAG, "Wifi set channel %d", channel);
  }
  vTaskDelete(NULL);
}
