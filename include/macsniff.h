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

uint16_t get_salt(void);
uint64_t macConvert(uint8_t *paddr);
esp_err_t macQueueInit(void);
void mac_process(void *pvParameters);
void IRAM_ATTR mac_add(uint8_t *paddr, int8_t rssi, snifftype_t sniff_type);
uint16_t mac_analyze(uint8_t *paddr, int8_t rssi, snifftype_t sniff_type);
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb);

#endif
