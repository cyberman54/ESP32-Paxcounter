#ifndef _LORAWAN_H
#define _LORAWAN_H

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>
#include "loraconf.h"

void onEvent(ev_t ev);
void gen_lora_deveui(uint8_t *pdeveui);
void RevBytes(unsigned char *b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void printKeys(void);
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb);
void lorawan_loop(void *pvParameters);

#endif