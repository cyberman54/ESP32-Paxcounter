#include "OTA.h"

const BintrayClient bintray(BINTRAY_USER, BINTRAY_REPO, BINTRAY_PACKAGE);

void ota_wifi_init(void) {
  const int RESPONSE_TIMEOUT_MS = 5000;
  unsigned long timeout = millis();

  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false)); // switch off monitor mode
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setHostname(PROGNAME);

/*
  while (WiFi.status() != WL_CONNECTED) {
    ESP_LOGI(TAG, "WiFi Status %d", WiFi.status());
    if (millis() - timeout > RESPONSE_TIMEOUT_MS) {
      ESP_LOGE(TAG, "WiFi connection timeout. Please check your settings!");
    }

    delay(500);
  }

  configASSERT(WiFi.isConnected() == true);
*/

}