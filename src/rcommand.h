#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include "senddata.h"
#include "configmanager.h"
#include "lorawan.h"
#include "macsniff.h"

// table of remote commands and assigned functions
typedef struct {
  const uint8_t opcode;
  void (*func)(uint8_t []);
  const bool store;
} cmd_t;

void rcommand(uint8_t cmd[], uint8_t cmdlength);
void switch_lora(uint8_t sf, uint8_t tx);

#endif