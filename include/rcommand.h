#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include "senddata.h"
#include "cyclic.h"
#include "configmanager.h"
#include "lorawan.h"
#include "macsniff.h"
#include <rom/rtc.h>
#include "cyclic.h"
#include "timekeeper.h"
#ifdef DBTIMESYNC
#include "DBtimesync.h"
#endif

// table of remote commands and assigned functions
typedef struct {
  const uint8_t opcode;
  void (*func)(uint8_t []);
  uint8_t params;
  const bool store;
} cmd_t;

void rcommand(uint8_t cmd[], uint8_t cmdlength);
void do_reset();

#endif