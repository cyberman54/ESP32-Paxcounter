// Basic Config
#include "globals.h"
#include "wifiscan.h"
#include <esp_coexist.h>
#include "coexist_internal.h"

// Local logging tag
static const char TAG[] = "wifi";

TimerHandle_t WifiChanTimer;

static wifi_country_t wifi_country = {WIFI_MY_COUNTRY, WIFI_CHANNEL_MIN,
                                      WIFI_CHANNEL_MAX, 100,
                                      WIFI_COUNTRY_POLICY_MANUAL};

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

// Software-timer driven Wifi channel rotation callback function
void switchWifiChannel(TimerHandle_t xTimer) {
  channel =
      (channel % WIFI_CHANNEL_MAX) + 1; // rotate channel 1..WIFI_CHANNEL_MAX
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_init(void) {
  wifi_init_config_t wificfg = WIFI_INIT_CONFIG_DEFAULT();
  wificfg.nvs_enable = 0;        // we don't need any wifi settings from NVRAM
  wificfg.wifi_task_core_id = 0; // we want wifi task running on core 0

  // wifi_promiscuous_filter_t filter = {
  //    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT}; // only MGMT frames
  // .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL}; // we use all frames

  wifi_promiscuous_filter_t filter = {.filter_mask =
                                          WIFI_PROMIS_FILTER_MASK_MGMT |
                                          WIFI_PROMIS_FILTER_MASK_DATA};

  ESP_ERROR_CHECK(esp_wifi_init(&wificfg)); // configure Wifi with cfg
  ESP_ERROR_CHECK(
      esp_wifi_set_country(&wifi_country)); // set locales for RF and channels
  ESP_ERROR_CHECK(
      esp_wifi_set_storage(WIFI_STORAGE_RAM)); // we don't need NVRAM
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // no modem power saving
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter)); // set frame filter
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); // now switch on monitor mode

  // setup wifi channel rotation timer
  WifiChanTimer =
      xTimerCreate("WifiChannelTimer", pdMS_TO_TICKS(cfg.wifichancycle * 10),
                   pdTRUE, (void *)0, switchWifiChannel);
  assert(WifiChanTimer);
  xTimerStart(WifiChanTimer, 0);
}