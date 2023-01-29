#include <wifiManager.h>

bool connectWifi() {
  
  stopWifiScan();
  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE,
              INADDR_NONE); // call is only a workaround for bug in WiFi class
  // see https://github.com/espressif/arduino-esp32/issues/806
  WiFi.setHostname(clientId);
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  // Connect to WiFi network
  // workaround applied here to bypass WIFI_AUTH failure
  // see https://github.com/espressif/arduino-esp32/issues/2501

  // 1st try
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() == WL_DISCONNECTED) {
    delay(500);
  }
  // 2nd try
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(500);
  }

  uint8_t i = WIFI_MAX_TRY;

  while (i--) {
    ESP_LOGI(TAG, "Trying to connect to %s, attempt %u of %u", WIFI_SSID,
             WIFI_MAX_TRY - i, WIFI_MAX_TRY);
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(3000); // wait for stable connect
    WiFi.reconnect();
  }
  return false;
}

void disconnectWifi() {}

void stopWifiScan() {
  uint8_t val[] = {0};
  set_wifiscan(val);
}

void startWifiScan() {
  uint8_t val[] = {1};
  set_wifiscan(val);
}
