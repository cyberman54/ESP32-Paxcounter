#if (USE_OTA)

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

#include "ota.h"

using namespace std;

const BintrayClient paxexpress(PAXEXPRESS_USER, PAXEXPRESS_REPO,
                               PAXEXPRESS_PACKAGE);
// usage of paxexpress: see https://github.com/paxexpress/docs

// Connection port (HTTPS)
const int port = 443;

// Variables to validate firmware content
int volatile contentLength = 0;
bool volatile isValidContentType = false;


// helper function to extract header value from header
inline String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

void start_ota_update() {
  const char *host = clientId;

  switch_LED(LED_ON);

// init display
#ifdef HAS_DISPLAY

  dp_setup();
  dp_setFont(MY_FONT_NORMAL);
  dp->printf("SOFTWARE UPDATE\r\n");
  dp->printf("WiFi connect  ..\r\n");
  dp->printf("Has Update?   ..\r\n");
  dp->printf("Fetching      ..\r\n");
  dp->printf("Downloading   ..\r\n");
  dp->printf("Rebooting     ..\r\n");
  dp_dump();
#endif

  ESP_LOGI(TAG, "Starting Wifi OTA update");
  ota_display(1, "**", WIFI_SSID);

  WiFi.disconnect(true);
  WiFi.config(INADDR_NONE, INADDR_NONE,
              INADDR_NONE); // call is only a workaround for bug in WiFi class
  // see https://github.com/espressif/arduino-esp32/issues/806
  WiFi.setHostname(host);
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
  int ret = 1; // 0 = finished, 1 = retry, -1 = abort

  while (i--) {
    ESP_LOGI(TAG, "Trying to connect to %s, attempt %u of %u", WIFI_SSID,
             WIFI_MAX_TRY - i, WIFI_MAX_TRY);
    if (WiFi.status() == WL_CONNECTED) {
      // we now have wifi connection and try to do an OTA over wifi update
      ESP_LOGI(TAG, "Connected to %s", WIFI_SSID);
      ota_display(1, "OK", "WiFi connected");
      // do a number of tries to update firmware limited by OTA_MAX_TRY
      uint8_t j = OTA_MAX_TRY;
      while ((j--) && (ret > 0)) {
        ESP_LOGI(TAG, "Starting OTA update, attempt %u of %u", OTA_MAX_TRY - j,
                 OTA_MAX_TRY);
        ret = do_ota_update();
      }
      if (WiFi.status() == WL_CONNECTED)
        goto end; // OTA update finished or OTA max attemps reached
    }
    delay(10000); // wait for stable connect
    WiFi.reconnect();
  }

  // wifi did not connect
  ESP_LOGI(TAG, "Could not connect to %s", WIFI_SSID);
  ota_display(1, " E", "no WiFi connect");
  delay(5000);

end:
  switch_LED(LED_OFF);
  ESP_LOGI(TAG, "Rebooting to %s firmware", (ret == 0) ? "new" : "current");
  ota_display(5, "**", ""); // mark line rebooting
  delay(5000);
  do_reset(false);
} // start_ota_update

// Reads data vom wifi client and flashes it to ota partition
// returns: 0 = finished, 1 = retry, -1 = abort
int do_ota_update() {
  char buf[17];
  bool redirect = true;
  size_t written = 0;

  // Fetch the latest firmware version
  ESP_LOGI(TAG, "Checking latest firmware version on server");
  ota_display(2, "**", "checking version");

  if (WiFi.status() != WL_CONNECTED)
    return 1;

  const String latest = paxexpress.getLatestVersion();

  if (latest.length() == 0) {
    ESP_LOGI(TAG, "Could not fetch info on latest firmware");
    ota_display(2, " E", "file not found");
    return -1;
  } else if (version_compare(latest, cfg.version) <= 0) {
    ESP_LOGI(TAG, "Current firmware is up to date");
    ota_display(2, "NO", "no update found");
    return -1;
  }
  ESP_LOGI(TAG, "New firmware version v%s available", latest.c_str());
  ota_display(2, "OK", latest.c_str());

  ota_display(3, "**", "");
  if (WiFi.status() != WL_CONNECTED)
    return 1;
  String firmwarePath = paxexpress.getBinaryPath(latest);
  if (!firmwarePath.endsWith(".bin")) {
    ESP_LOGI(TAG, "Unsupported binary format");
    ota_display(3, " E", "file type error");
    return -1;
  }

  String currentHost = paxexpress.getStorageHost();
  String prevHost = currentHost;

  WiFiClientSecure client;

  client.setTimeout(RESPONSE_TIMEOUT_MS);
  client.setInsecure();
  if (!client.connect(currentHost.c_str(), port)) {
    ESP_LOGI(TAG, "Cannot connect to %s", currentHost.c_str());
    ota_display(3, " E", "connection lost");
    goto abort;
  }

  while (redirect) {
    if (currentHost != prevHost) {
      client.stop();
      if (!client.connect(currentHost.c_str(), port)) {
        ESP_LOGI(TAG, "Redirect detected, but cannot connect to %s",
                 currentHost.c_str());
        ota_display(3, " E", "server error");
        goto abort;
      }
    }

    ESP_LOGI(TAG, "Requesting %s", firmwarePath.c_str());

    client.print(String("GET ") + firmwarePath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + currentHost + "\r\n");
    client.print("Cache-Control: no-cache\r\n");
    client.print("Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if ((long)(millis() - timeout) > (RESPONSE_TIMEOUT_MS)) {
        ESP_LOGI(TAG, "Client timeout");
        ota_display(3, " E", "client timeout");
        goto abort;
      }
    }

    while (client.available()) {
      String line = client.readStringUntil('\n');
      String lineLowerCase = line;
      lineLowerCase.toLowerCase();
      // Check if the line is end of headers by removing space symbol
      line.trim();
      // if the the line is empty, this is the end of the headers
      if (!line.length()) {
        break; // proceed to OTA update
      }

      // Check allowed HTTP responses
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") > 0) {
          ESP_LOGI(TAG, "Got 200 status code from server. Proceeding to "
                        "firmware flashing");
          redirect = false;
        } else if (line.indexOf("302") > 0) {
          ESP_LOGI(TAG, "Got 302 status code from server. Redirecting to "
                        "new address");
          redirect = true;
        } else {
          ESP_LOGI(TAG, "Could not get firmware download URL");
          goto retry;
        }
      }

      // Extracting new redirect location
      if (lineLowerCase.startsWith("location: ")) {
        String newUrl = getHeaderValue(line, "location: ");
        ESP_LOGI(TAG, "Got new url: %s", newUrl.c_str());
        newUrl.remove(0, newUrl.indexOf("//") + 2);
        currentHost = newUrl.substring(0, newUrl.indexOf('/'));
        newUrl.remove(newUrl.indexOf(currentHost), currentHost.length());
        firmwarePath = newUrl;
        continue;
      }

      // Checking headers
      if (lineLowerCase.startsWith("content-length: ")) {
        contentLength =
            atoi((getHeaderValue(line, "content-length: ")).c_str());
        ESP_LOGI(TAG, "Got %d bytes from server", contentLength);
      }

      if (lineLowerCase.startsWith("content-type: ")) {
        String contentType = getHeaderValue(line, "content-type: ");
        ESP_LOGI(TAG, "Got %s payload", contentType.c_str());
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    } // while (client.available())
  }   // while (redirect)

  ota_display(3, "OK", ""); // line download

  // check whether we have everything for OTA update
  if (!(contentLength && isValidContentType)) {
    ESP_LOGI(TAG, "Invalid OTA server response");
    ota_display(4, " E", "response error");
    goto retry;
  }

#if (HAS_LED != NOT_A_PIN)
#ifndef LED_ACTIVE_LOW
  if (!Update.begin(contentLength, U_FLASH, HAS_LED, HIGH)) {
#else
  if (!Update.begin(contentLength, U_FLASH, HAS_LED, LOW)) {
#endif
#else
  if (!Update.begin(contentLength)) {
#endif
    ESP_LOGI(TAG, "Not enough space to start OTA update");
    ota_display(4, " E", "disk full");
    goto abort;
  }

#ifdef HAS_DISPLAY
  // register callback function for showing progress while streaming data
  Update.onProgress(&show_progress);
#endif

  ota_display(4, "**", "writing...");
  written = Update.writeStream(client); // this is a blocking call

  if (written == contentLength) {
    ESP_LOGI(TAG, "Written %u bytes successfully", written);
    snprintf(buf, 17, "%ukB Done!", (uint16_t)(written / 1024));
    ota_display(4, "OK", buf);
  } else {
    ESP_LOGI(TAG, "Written only %u of %u bytes, OTA update attempt cancelled",
             written, contentLength);
  }

  if (Update.end()) {
    goto finished;
  } else {
    ESP_LOGI(TAG, "An error occurred. Error#: %d", Update.getError());
    snprintf(buf, 17, "Error#: %d", Update.getError());
    ota_display(4, " E", buf);
    goto retry;
  }

finished:
  client.stop();
  ESP_LOGI(TAG, "OTA update finished");
  return 0;

abort:
  client.stop();
  ESP_LOGI(TAG, "OTA update failed");
  return -1;

retry:
  return 1;
} // do_ota_update

void ota_display(const uint8_t row, const std::string status,
                 const std::string msg) {
#ifdef HAS_DISPLAY
  dp->setCursor(14 * 8, row * 8);
  dp->printf(status.substr(0, 2).c_str());
  if (!msg.empty()) {
    dp->printf("                ");
    dp->printf(msg.substr(0, 16).c_str());
    // dp->printf("\r\n");
  }
  dp_dump();
#endif
}

// callback function to show download progress while streaming data
void show_progress(unsigned long current, unsigned long size) {
#ifdef HAS_DISPLAY
  char buf[17];
  snprintf(buf, 17, "%-9lu (%3lu%%)", current, current * 100 / size);
  ota_display(4, "**", buf);
#endif
}

#endif // USE_OTA