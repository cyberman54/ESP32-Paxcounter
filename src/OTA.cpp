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

#include <WiFiClientSecure.h>
#include <Update.h>
#include <BintrayClient.h>
#include "ota.h"

const BintrayClient bintray(BINTRAY_USER, BINTRAY_REPO, BINTRAY_PACKAGE);

// Connection port (HTTPS)
const int port = 443;

// Connection timeout
const uint32_t RESPONSE_TIMEOUT_MS = 5000;

// Variables to validate firmware content
volatile int contentLength = 0;
volatile bool isValidContentType = false;

// Local logging tag
static const char TAG[] = "main";

void start_ota_update() {
  ota_update = false; // clear ota trigger switch

  ESP_LOGI(TAG, "Stopping Wifi scanner");
  vTaskDelete(WifiLoopTask);

  ESP_LOGI(TAG, "Starting Wifi OTA update");
  // switch off monitor more
  ESP_ERROR_CHECK(
      esp_wifi_set_promiscuous(false)); // now switch on monitor mode
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL));

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    ESP_LOGI(TAG, "trying to connect to %s", WIFI_SSID);
  }

  ESP_LOGI(TAG, "connected to %s", WIFI_SSID);

  checkFirmwareUpdates();
  ESP.restart(); // reached only if update was not successful

} // start_ota_update

void checkFirmwareUpdates() {
  // Fetch the latest firmware version
  ESP_LOGI(TAG, "Checking latest firmware version...");
  const String latest = bintray.getLatestVersion();
  if (latest.length() == 0) {
    ESP_LOGI(TAG, "Could not load info about the latest firmware, so nothing "
                  "to update. Continue ...");
    return;
  } else if (atoi(latest.c_str()) <= VERSION) {
    ESP_LOGI(TAG, "The current firmware is up to date. Continue ...");
    return;
  }

  ESP_LOGI(TAG, "There is a new version of firmware available: v.%s",
           latest.c_str());
  processOTAUpdate(latest);
}

// A helper function to extract header value from header
inline String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

/**
 * OTA update processing
 */
void processOTAUpdate(const String &version) {
  String firmwarePath = bintray.getBinaryPath(version);
  if (!firmwarePath.endsWith(".bin")) {
    ESP_LOGI(TAG, "Unsupported binary format. OTA update cannot be performed!");
    return;
  }

  String currentHost = bintray.getStorageHost();
  String prevHost = currentHost;

  WiFiClientSecure client;
  client.setCACert(bintray.getCertificate(currentHost));

  if (!client.connect(currentHost.c_str(), port)) {
    ESP_LOGI(TAG, "Cannot connect to %s", currentHost.c_str());
    return;
  }

  bool redirect = true;
  while (redirect) {
    if (currentHost != prevHost) {
      client.stop();
      client.setCACert(bintray.getCertificate(currentHost));
      if (!client.connect(currentHost.c_str(), port)) {
        ESP_LOGI(TAG,
                 "Redirect detected! Cannot connect to %s for some reason!",
                 currentHost.c_str());
        return;
      }
    }

    // ESP_LOGI(TAG, "Requesting: " + firmwarePath);

    client.print(String("GET ") + firmwarePath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + currentHost + "\r\n");
    client.print("Cache-Control: no-cache\r\n");
    client.print("Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > RESPONSE_TIMEOUT_MS) {
        ESP_LOGI(TAG, "Client Timeout !");
        client.stop();
        return;
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
          ESP_LOGI(TAG, "Could not get a valid firmware url");
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
        ESP_LOGI(TAG, "firmwarePath: %s", firmwarePath.c_str());
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
  }

  // check whether we have everything for OTA update
  if (contentLength && isValidContentType) {
    if (Update.begin(contentLength)) {
      ESP_LOGI(TAG, "Starting Over-The-Air update. This may take some time to "
                    "complete ...");
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        ESP_LOGI(TAG, "Written %d successfully", written);
      } else {
        ESP_LOGI(TAG, "Written only %d / %d Retry?", written, contentLength);
        // Retry??
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          ESP_LOGI(TAG, "OTA update has successfully completed. Rebooting ...");
          ESP.restart();
        } else {
          ESP_LOGI(TAG, "Something went wrong! OTA update hasn't been finished "
                        "properly.");
        }
      } else {
        ESP_LOGI(TAG, "An error occurred. Error #: %d", Update.getError());
      }
    } else {
      ESP_LOGI(TAG, "There isn't enough space to start OTA update");
      client.flush();
    }
  } else {
    ESP_LOGI(TAG,
             "There was no valid content in the response from the OTA server!");
    client.flush();
  }
}