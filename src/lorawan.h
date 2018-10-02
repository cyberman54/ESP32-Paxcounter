#ifndef _LORAWAN_H
#define _LORAWAN_H

#include "globals.h"
#include "rcommand.h"

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>
#include "loraconf.h"

// Needed for 24AA02E64, does not hurt anything if included and not used
#ifdef MCP_24AA02E64_I2C_ADDRESS
#include <Wire.h>
#endif

extern QueueHandle_t LoraSendQueue;

void onEvent(ev_t ev);
void gen_lora_deveui(uint8_t *pdeveui);
void RevBytes(unsigned char *b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void showLoraKeys(void);
void switch_lora(uint8_t sf, uint8_t tx);
void initlmic(osjob_t *j);

#endif