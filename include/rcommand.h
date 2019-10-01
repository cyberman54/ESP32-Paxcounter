#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include "senddata.h"
#include "cyclic.h"
#include "configmanager.h"
#if(HAS_LORA)
#include "lorawan.h"
#endif
#include "macsniff.h"
#include <rom/rtc.h>
#include "cyclic.h"
#include "timekeeper.h"
#if(TIME_SYNC_LORASERVER)
#include "timesync.h"
#endif

// table of remote commands and assigned functions
typedef struct {
  const uint8_t opcode;
  void (*func)(uint8_t []);
  const uint8_t params;
  const bool store;
} cmd_t;

void rcommand(const uint8_t cmd[], const uint8_t cmdlength);
void do_reset();

#endif
