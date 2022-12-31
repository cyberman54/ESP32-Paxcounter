#if (HAS_LORA)

#ifndef _LORAWAN_H
#define _LORAWAN_H

#include "globals.h"
#include "rcommand.h"
#include "timekeeper.h"
#include <driver/rtc_io.h>

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
extern char lmic_event_msg[LMIC_EVENTMSG_LEN]; // display buffer

esp_err_t lmic_init(void);
void lora_setupForNetwork(bool preJoin);
void SaveLMICToRTC(uint32_t deepsleep_sec);
void LoadLMICFromRTC();
void lmictask(void *pvParameters);
void gen_lora_deveui(uint8_t *pdeveui);
void RevBytes(unsigned char *b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);
void os_getDevKey(u1_t *buf);
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void lora_send(void *pvParameters);
void lora_enqueuedata(MessageBuffer_t *message);
void lora_queuereset(void);
void lora_waitforidle(uint16_t timeout_sec);
uint32_t lora_queuewaiting(void);
void myEventCallback(void *pUserData, ev_t ev);
void myRxCallback(void *pUserData, uint8_t port, const uint8_t *pMsg,
                            size_t nMsg);
void myTxCallback(void *pUserData, int fSuccess);
const char *getSfName(rps_t rps);
const char *getBwName(rps_t rps);
const char *getCrName(rps_t rps);

#if (VERBOSE)
void showLoraKeys(void);
#endif // VERBOSE

#endif

#endif // HAS_LORA