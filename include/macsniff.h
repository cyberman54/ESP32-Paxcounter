#ifndef _MACSNIFF_H
#define _MACSNIFF_H

// ESP32 Functions
#include <esp_wifi.h>

// Hash function for scrambling MAC addresses
#include "hash.h"
#include "senddata.h"
#include "cyclic.h"

#define MAC_SNIFF_WIFI 0
#define MAC_SNIFF_BLE 1

uint16_t get_salt(void);
uint64_t macConvert(uint8_t *paddr);
bool mac_add(uint8_t *paddr, int8_t rssi, bool sniff_type);
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb);

#endif