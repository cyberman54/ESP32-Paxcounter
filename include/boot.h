#ifndef BOOT_H
#define BOOT_H

#include "globals.h"
#include "hash.h"

#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

void IRAM_ATTR exit_boot_menu(void);
void start_boot_menu(void);

#endif // BOOT_H
