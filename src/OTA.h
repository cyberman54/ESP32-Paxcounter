#ifndef OTA_H
#define OTA_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <BintrayClient.h>

void checkFirmwareUpdates();
void processOTAUpdate(const String &version);
void start_ota_update();

#endif // OTA_H