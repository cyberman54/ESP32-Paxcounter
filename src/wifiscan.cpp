// Basic Config
#include "globals.h"
#include "wifiscan.h"

// Local logging tag
static const char TAG[] = "wifi";

TimerHandle_t WifiChanTimer;

typedef struct {
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; // receiver address
  uint8_t addr2[6]; // sender address
  uint8_t addr3[6]; // filtering address
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; // optional
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; // network data ended with 4 bytes csum (CRC32)
} wifi_ieee80211_packet_t;

// using IRAM_ATTR here to speed up callback function
IRAM_ATTR void wifi_sniffer_packet_handler(void *buff,
                                           wifi_promiscuous_pkt_type_t type) {

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt =
      (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

// process seen MAC
#if MACFILTER
  // we guess it's a smartphone, if U/L bit #2 of MAC is set
  // bit #2 = 1 -> local mac (randomized) / bit #2 = 0 -> universal mac
  if ((hdr->addr2[0] & 0b10) == 0)
    return;
  else
#endif
    mac_add((uint8_t *)hdr->addr2, ppkt->rx_ctrl.rssi, MAC_SNIFF_WIFI);
}

// Software-timer driven Wifi channel rotation callback function
void switchWifiChannel(TimerHandle_t xTimer) {
  channel =
      (channel % WIFI_CHANNEL_MAX) + 1; // rotate channel 1..WIFI_CHANNEL_MAX
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_init(void) {

  wifi_country_t wifi_country = {WIFI_MY_COUNTRY, WIFI_CHANNEL_MIN,
                                 WIFI_CHANNEL_MAX, 100,
                                 WIFI_COUNTRY_POLICY_MANUAL};

  wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_cfg.event_handler = NULL;  // we don't need a wifi event handler
  wifi_cfg.nvs_enable = 0;        // we don't need any wifi settings from NVRAM
  wifi_cfg.wifi_task_core_id = 0; // we want wifi task running on core 0

  wifi_promiscuous_filter_t wifi_filter = {.filter_mask =
                                               WIFI_PROMIS_FILTER_MASK_MGMT |
                                               WIFI_PROMIS_FILTER_MASK_DATA};

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg)); // start Wifi task
  ESP_ERROR_CHECK(
      esp_wifi_set_country(&wifi_country)); // set locales for RF and channels
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // no modem power saving

  ESP_ERROR_CHECK(
      esp_wifi_set_promiscuous_filter(&wifi_filter)); // set frame filter
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); // now switch on monitor mode

  // setup wifi channel hopping timer
  WifiChanTimer =
      xTimerCreate("WifiChannelTimer",
                   (cfg.wifichancycle > 0) ? pdMS_TO_TICKS(cfg.wifichancycle)
                                           : pdMS_TO_TICKS(50),
                   pdTRUE, (void *)0, switchWifiChannel);
  // start timer
  if (cfg.wifichancycle > 0)
    xTimerStart(WifiChanTimer, (TickType_t)0);
}

void switch_wifi_sniffer(uint8_t state) {
  if (state) {
    // switch wifi sniffer on
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(WIFI_CHANNEL_MIN, WIFI_SECOND_CHAN_NONE);
    if (cfg.wifichancycle > 0)
      xTimerStart(WifiChanTimer, (TickType_t)0);
  } else {
    // switch wifi sniffer off
    if (xTimerIsTimerActive(WifiChanTimer) != pdFALSE)
      xTimerStop(WifiChanTimer, (TickType_t)0);
    esp_wifi_set_promiscuous(false);
    ESP_ERROR_CHECK(esp_wifi_stop());
  }
}