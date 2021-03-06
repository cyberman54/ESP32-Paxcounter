#ifndef OTA_H
#define OTA_H

#ifdef USE_OTA

#include "globals.h"
#include "led.h"
#include "display.h"
#include "configmanager.h"

#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BintrayClient.h>

int do_ota_update();
void start_ota_update();
void ota_display(const uint8_t row, const std::string status,
                 const std::string msg);
void show_progress(unsigned long current, unsigned long size);

#endif // USE_OTA

#endif // OTA_H