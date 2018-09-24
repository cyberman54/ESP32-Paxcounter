#ifndef OTA_H
#define OTA_H

#ifdef USE_OTA

#include "globals.h"
#include "update.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BintrayClient.h>
#include <string>

void do_ota_update();
void start_ota_update();
int version_compare(const String v1, const String v2);
void show_progress(size_t current, size_t size);
void display(const uint8_t row, const std::string status, const std::string msg);

#endif // USE_OTA

#endif // OTA_H
