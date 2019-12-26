#ifndef _LORAWAN_H
#define _LORAWAN_H

#include "globals.h"
#include "rcommand.h"
#include "timekeeper.h"
#include <driver/rtc_io.h>
#if (TIME_SYNC_LORASERVER)
#include "timesync.h"
#endif

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <arduino_lmic_hal_boards.h>
#include "loraconf.h"

// Needed for 24AA02E64, does not hurt anything if included and not used
#ifdef MCP_24AA02E64_I2C_ADDRESS
#include <Wire.h>
#endif

extern TaskHandle_t lmicTask, lorasendTask;

// table of LORAWAN MAC commands
typedef struct {
  const uint8_t opcode;
  const char cmdname[20];
  const uint8_t params;
} mac_t;

esp_err_t lora_stack_init(bool do_join);
void lora_setupForNetwork(bool preJoin);
void lmictask(void *pvParameters);
void gen_lora_deveui(uint8_t *pdeveui);
void RevBytes(unsigned char *b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void showLoraKeys(void);
void lora_send(void *pvParameters);
void lora_enqueuedata(MessageBuffer_t *message);
void lora_queuereset(void);
void IRAM_ATTR myEventCallback(void *pUserData, ev_t ev);
void IRAM_ATTR myRxCallback(void *pUserData, uint8_t port,
                                   const uint8_t *pMsg, size_t nMsg);
void IRAM_ATTR myTxCallback(void *pUserData, int fSuccess);
void mac_decode(const uint8_t cmd[], const uint8_t cmdlen, const mac_t table[],
                const uint8_t tablesize);
uint8_t getBattLevel(void);
const char *getSfName(rps_t rps);
const char *getBwName(rps_t rps);
const char *getCrName(rps_t rps);

#if (TIME_SYNC_LORAWAN)
void user_request_network_time_callback(void *pVoidUserUTCTime,
                                               int flagSuccess);
#endif

#endif