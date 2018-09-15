#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include "globals.h"

void checkFirmwareUpdates();
void processOTAUpdate(const String &version);
void start_ota_update();

#endif // OTA_H