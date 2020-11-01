#ifndef _MACSNIFF_H
#define _MACSNIFF_H

// ESP32 Functions
#include <esp_wifi.h>

// Hash function for scrambling MAC addresses
#include "hash.h"
#include "senddata.h"
#include "cyclic.h"
#include "led.h"

#if (COUNT_ENS)
#include "corona.h"
#endif

#define MAC_SNIFF_WIFI 0
#define MAC_SNIFF_BLE 1
#define MAC_SNIFF_BLE_CWA 2

uint16_t get_salt(void);
uint64_t macConvert(uint8_t *paddr);
uint16_t mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type);
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb);

#endif
