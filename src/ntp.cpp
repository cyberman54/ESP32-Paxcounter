/*
 Parts of this code:
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "ntp.h"

const char* ntpServer = "pool.ntp.org";


void set_network_time() {
  const char *host = clientId;

  ESP_LOGI(TAG, "Starting Wifi for time synchronization");

  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE,
              INADDR_NONE); // call is only a workaround for bug in WiFi class
  // see https://github.com/espressif/arduino-esp32/issues/806
  WiFi.setHostname(host);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
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

  if (WiFi.status() == WL_CONNECTED) {
    // we now have wifi connection and try to do an time_sync over wifi update
    ESP_LOGI(TAG, "Connected to %s", WIFI_SSID);

    // set time server while you are connected to wife
    long current_time = 0L;
    unsigned long measured_at = 0L;
    
    if(queryNTP(ntpServer, current_time, measured_at)){
      setTime(current_time);
      ESP_LOGI(TAG, "Set local date time to %s", dateTime().c_str());
    } else{
      ESP_LOGI(TAG, "Could not connect to ntp server %s", ntpServer);  
    }
  } else {
    // wifi did not connect
    ESP_LOGI(TAG, "Could not connect to %s", WIFI_SSID);
  }
}
