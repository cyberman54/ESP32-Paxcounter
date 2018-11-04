#ifdef USE_OTA

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

const BintrayClient bintray(BINTRAY_USER, BINTRAY_REPO, BINTRAY_PACKAGE);

// Connection port (HTTPS)
const int port = 443;

// Variables to validate firmware content
int volatile contentLength = 0;
bool volatile isValidContentType = false;

// Local logging tag
static const char TAG[] = "main";

// helper function to extract header value from header
inline String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

void start_ota_update() {

  /*
  // check battery status if we can before doing ota
  #ifdef HAS_BATTERY_PROBE
    if (!batt_sufficient()) {
      ESP_LOGW(TAG, "Battery voltage %dmV too low for OTA", batt_voltage);
      return;
    }
  #endif
  */

  switch_LED(LED_ON);

#ifdef HAS_DISPLAY
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
#ifdef DISPLAY_FLIP
  u8x8.setFlipMode(1);
#endif
  u8x8.setInverseFont(1);
  u8x8.print("SOFTWARE UPDATE \n");
  u8x8.setInverseFont(0);
  u8x8.print("WiFi connect  ..\n");
  u8x8.print("Has Update?   ..\n");
  u8x8.print("Fetching      ..\n");
  u8x8.print("Downloading   ..\n");
  u8x8.print("Rebooting     ..");
#endif

  ESP_LOGI(TAG, "Starting Wifi OTA update");
  display(1, "**", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int i = WIFI_MAX_TRY, j = OTA_MAX_TRY;
  bool ret = false;

  while (i--) {
    ESP_LOGI(TAG, "Trying to connect to %s", WIFI_SSID);
    if (WiFi.status() == WL_CONNECTED) {
      // we now have wifi connection and try to do an OTA over wifi update
      ESP_LOGI(TAG, "Connected to %s", WIFI_SSID);
      display(1, "OK", "WiFi connected");
      // do a number of tries to update firmware limited by OTA_MAX_TRY
      while ( j--) {
        ESP_LOGI(TAG,
                 "Starting OTA update, attempt %u of %u. This will take some "
                 "time to complete...",
                 OTA_MAX_TRY - j, OTA_MAX_TRY);
        ret = do_ota_update();
        if (ret)
          goto end; // update successful
      }
      goto end; // update not successful
    }
  }

  // wifi did not connect
  ESP_LOGI(TAG, "Could not connect to %s", WIFI_SSID);
  display(1, " E", "no WiFi connect");
  vTaskDelay(5000 / portTICK_PERIOD_MS);

end:
  switch_LED(LED_OFF);
  ESP_LOGI(TAG, "Rebooting to runmode using %s firmware",
           ret ? "new" : "current");
  display(5, "**", ""); // mark line rebooting
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  ESP.restart();

} // start_ota_update

bool do_ota_update() {

  char buf[17];
  bool redirect = true;
  size_t written = 0;

  // Fetch the latest firmware version
  ESP_LOGI(TAG, "Checking latest firmware version on server...");
  display(2, "**", "checking version");
  const String latest = bintray.getLatestVersion();

  if (latest.length() == 0) {
    ESP_LOGI(
        TAG,
        "Could not load info about the latest firmware. Rebooting to runmode.");
    display(2, " E", "file not found");
    return false;
  } else if (version_compare(latest, cfg.version) <= 0) {
    ESP_LOGI(TAG, "Current firmware is up to date. Rebooting to runmode.");
    display(2, "NO", "no update found");
    return false;
  }
  ESP_LOGI(TAG, "New firmware version v%s available. Downloading...",
           latest.c_str());
  display(2, "OK", latest.c_str());

  display(3, "**", "");
  String firmwarePath = bintray.getBinaryPath(latest);
  if (!firmwarePath.endsWith(".bin")) {
    ESP_LOGI(TAG, "Unsupported binary format, OTA update cancelled.");
    display(3, " E", "file type error");
    return false;
  }

  String currentHost = bintray.getStorageHost();
  String prevHost = currentHost;

  WiFiClientSecure client;
  client.setCACert(bintray.getCertificate(currentHost));

  if (!client.connect(currentHost.c_str(), port)) {
    ESP_LOGI(TAG, "Cannot connect to %s", currentHost.c_str());
    display(3, " E", "connection lost");
    goto failure;
  }

  while (redirect) {
    if (currentHost != prevHost) {
      client.stop();
      client.setCACert(bintray.getCertificate(currentHost));
      if (!client.connect(currentHost.c_str(), port)) {
        ESP_LOGI(TAG, "Redirect detected, but cannot connect to %s",
                 currentHost.c_str());
        display(3, " E", "server error");
        goto failure;
      }
    }

    ESP_LOGI(TAG, "Requesting %s", firmwarePath.c_str());

    client.print(String("GET ") + firmwarePath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + currentHost + "\r\n");
    client.print("Cache-Control: no-cache\r\n");
    client.print("Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > RESPONSE_TIMEOUT_MS) {
        ESP_LOGI(TAG, "Client Timeout.");
        display(3, " E", "client timeout");
        goto failure;
      }
    }

    while (client.available()) {
      String line = client.readStringUntil('\n');
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
          ESP_LOGI(TAG, "Got 302 status code from server. Redirecting to the "
                        "new address");
          redirect = true;
        } else {
          ESP_LOGI(TAG, "Could not get a valid firmware url.");
          // Unexptected HTTP response. Retry or skip update?
          redirect = false;
        }
      }

      // Extracting new redirect location
      if (line.startsWith("Location: ")) {
        String newUrl = getHeaderValue(line, "Location: ");
        ESP_LOGI(TAG, "Got new url: %s", newUrl.c_str());
        newUrl.remove(0, newUrl.indexOf("//") + 2);
        currentHost = newUrl.substring(0, newUrl.indexOf('/'));
        newUrl.remove(newUrl.indexOf(currentHost), currentHost.length());
        firmwarePath = newUrl;
        continue;
      }

      // Checking headers
      if (line.startsWith("Content-Length: ")) {
        contentLength =
            atoi((getHeaderValue(line, "Content-Length: ")).c_str());
        ESP_LOGI(TAG, "Got %d bytes from server", contentLength);
      }

      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        ESP_LOGI(TAG, "Got %s payload", contentType.c_str());
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } // while (redirect)

  display(3, "OK", ""); // line download

  // check whether we have everything for OTA update
  if (!(contentLength && isValidContentType)) {
    ESP_LOGI(TAG,
             "There was no valid content in the response from the OTA server!");
    display(4, " E", "response error");
    goto failure;
  }

  if (!Update.begin(contentLength)) {
    ESP_LOGI(TAG, "There isn't enough space to start OTA update");
    display(4, " E", "disk full");
    goto failure;
  }

#ifdef HAS_DISPLAY
  // register callback function for showing progress while streaming data
  Update.onProgress(&show_progress);
#endif

  display(4, "**", "writing...");

  written = Update.writeStream(client);

  if (written == contentLength) {
    ESP_LOGI(TAG, "Written %u bytes successfully", written);
    snprintf(buf, 17, "%ukB Done!", (uint16_t)(written / 1024));
    display(4, "OK", buf);
  } else {
    ESP_LOGI(TAG, "Written only %u of %u bytes, OTA update attempt cancelled.",
             written, contentLength);
  }

  if (Update.end()) {
    goto finished;
  } else {
    ESP_LOGI(TAG, "An error occurred. Error #: %d", Update.getError());
    snprintf(buf, 17, "Error #: %d", Update.getError());
    display(4, " E", buf);
    goto failure;
  }

finished:
  client.stop();
  ESP_LOGI(TAG, "OTA update completed.");
  return true;

failure:
  client.stop();
  ESP_LOGI(TAG, "OTA update failed.");
  return false;

} // do_ota_update

void display(const uint8_t row, const std::string status,
             const std::string msg) {
#ifdef HAS_DISPLAY
  u8x8.setCursor(14, row);
  u8x8.print((status.substr(0, 2)).c_str());
  if (!msg.empty()) {
    u8x8.clearLine(7);
    u8x8.setCursor(0, 7);
    u8x8.print(msg.substr(0, 16).c_str());
  }
#endif
}

#ifdef HAS_DISPLAY
// callback function to show download progress while streaming data
void show_progress(unsigned long current, unsigned long size) {
  char buf[17];
  snprintf(buf, 17, "%-9lu (%3lu%%)", current, current * 100 / size);
  display(4, "**", buf);
}
#endif

// helper function to compare two versions. Returns 1 if v2 is
// smaller, -1 if v1 is smaller, 0 if equal

int version_compare(const String v1, const String v2) {
  //  vnum stores each numeric part of version
  int vnum1 = 0, vnum2 = 0;

  //  loop until both string are processed
  for (int i = 0, j = 0; (i < v1.length() || j < v2.length());) {
    //  storing numeric part of version 1 in vnum1
    while (i < v1.length() && v1[i] != '.') {
      vnum1 = vnum1 * 10 + (v1[i] - '0');
      i++;
    }

    //  storing numeric part of version 2 in vnum2
    while (j < v2.length() && v2[j] != '.') {
      vnum2 = vnum2 * 10 + (v2[j] - '0');
      j++;
    }

    if (vnum1 > vnum2)
      return 1;
    if (vnum2 > vnum1)
      return -1;

    //  if equal, reset variables and go for next numeric
    // part
    vnum1 = vnum2 = 0;
    i++;
    j++;
  }
  return 0;
}
#endif // USE_OTA