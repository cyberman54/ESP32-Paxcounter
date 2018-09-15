#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "globals.h"
#include <BintrayClient.h>
#include <WiFi.h>
#include "ota.h"
#include "SecureOTA.h"

void start_ota_update();

#endif // OTA_H