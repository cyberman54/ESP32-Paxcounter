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

esp_err_t lora_stack_init(bool do_join);
void lora_setupForNetwork(bool preJoin);
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
void IRAM_ATTR myEventCallback(void *pUserData, ev_t ev);
void IRAM_ATTR myRxCallback(void *pUserData, uint8_t port, const uint8_t *pMsg,
                            size_t nMsg);
void IRAM_ATTR myTxCallback(void *pUserData, int fSuccess);
const char *getSfName(rps_t rps);
const char *getBwName(rps_t rps);
const char *getCrName(rps_t rps);
// u1_t os_getBattLevel(void);

#if (VERBOSE)

// a table for storage of LORAWAN MAC commands
typedef struct {
  const uint8_t cid;
  const char cmdname[20];
  const uint8_t params;
} mac_t;

// table of LORAWAN MAC messages sent by the network to the device
// format: CDI, Command (max 19 chars), #parameters (bytes)
// source: LoRaWAN 1.1 Specification (October 11, 2017)
static const mac_t MACdn_table[] = {
    {0x01, "ResetConf", 1},          {0x02, "LinkCheckAns", 2},
    {0x03, "LinkADRReq", 4},         {0x04, "DutyCycleReq", 1},
    {0x05, "RXParamSetupReq", 4},    {0x06, "DevStatusReq", 0},
    {0x07, "NewChannelReq", 5},      {0x08, "RxTimingSetupReq", 1},
    {0x09, "TxParamSetupReq", 1},    {0x0A, "DlChannelReq", 4},
    {0x0B, "RekeyConf", 1},          {0x0C, "ADRParamSetupReq", 1},
    {0x0D, "DeviceTimeAns", 5},      {0x0E, "ForceRejoinReq", 2},
    {0x0F, "RejoinParamSetupReq", 1}};

static const uint8_t MACdn_tSize = sizeof(MACdn_table) / sizeof(MACdn_table[0]);

// table of LORAWAN MAC messages sent by the device to the network
static const mac_t MACup_table[] = {
    {0x01, "ResetInd", 1},        {0x02, "LinkCheckReq", 0},
    {0x03, "LinkADRAns", 1},      {0x04, "DutyCycleAns", 0},
    {0x05, "RXParamSetupAns", 1}, {0x06, "DevStatusAns", 2},
    {0x07, "NewChannelAns", 1},   {0x08, "RxTimingSetupAns", 0},
    {0x09, "TxParamSetupAns", 0}, {0x0A, "DlChannelAns", 1},
    {0x0B, "RekeyInd", 1},        {0x0C, "ADRParamSetupAns", 0},
    {0x0D, "DeviceTimeReq", 0},   {0x0F, "RejoinParamSetupAns", 1}};

static const uint8_t MACup_tSize = sizeof(MACup_table) / sizeof(MACup_table[0]);

void mac_decode(const uint8_t cmd[], const uint8_t cmdlen, bool is_down);
void showLoraKeys(void);
#endif // VERBOSE

#endif