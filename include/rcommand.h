#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include <rom/rtc.h>

#include "libpax_helpers.h"
#include "senddata.h"
#include "cyclic.h"
#include "configmanager.h"
#include "lorawan.h"
#include "sensor.h"
#include "cyclic.h"
#include "timekeeper.h"
#include "esp_sntp.h"
#include "timesync.h"
#include "power.h"
#include "antenna.h"
#include "payload.h"

// maximum number of elements in rcommand interpreter queue
#define RCMD_QUEUE_SIZE 5

extern TaskHandle_t rcmdTask;

// table of remote commands and assigned functions
typedef struct {
  const uint8_t opcode;
  void (*func)(uint8_t[]);
  const uint8_t params;
} cmd_t;

// Struct for remote command processing queue
typedef struct {
  uint8_t cmd[10];
  uint8_t cmdLen;
} RcmdBuffer_t;

void rcommand(const uint8_t *cmd, const size_t cmdlength);
void rcmd_queuereset(void);
uint32_t rcmd_queuewaiting(void);
void rcmd_deinit(void);
esp_err_t rcmd_init(void);

#endif
