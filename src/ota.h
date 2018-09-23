#ifndef OTA_H
#define OTA_H

#include "globals.h"
#include "update.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BintrayClient.h>
#include <string>

void checkFirmwareUpdates();
void processOTAUpdate(const String &version);
void start_ota_update();
int version_compare(const String v1, const String v2);
void show_progress(size_t current, size_t size);

#endif // OTA_H
