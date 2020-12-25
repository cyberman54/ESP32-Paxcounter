#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include <rom/rtc.h>

#include "senddata.h"
#include "cyclic.h"
#include "configmanager.h"
#include "lorawan.h"
#include "sensor.h"
#include "macsniff.h"
#include "wifiscan.h"
#include "cyclic.h"
#include "timekeeper.h"
#include "timesync.h"
#include "blescan.h"

// table of remote commands and assigned functions
typedef struct {
  const uint8_t opcode;
  void (*func)(uint8_t[]);
  const uint8_t params;
  const bool store;
} cmd_t;

extern bool rcmd_busy;

void rcommand(const uint8_t cmd[], const uint8_t cmdlength);
void do_reset(bool warmstart);

#endif
