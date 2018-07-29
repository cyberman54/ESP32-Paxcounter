#ifndef _RCOMMAND_H
#define _RCOMMAND_H

#include "senddata.h"
#include "configmanager.h"

// table of remote commands and assigned functions
typedef struct {
  const uint8_t nam;
  void (*func)(uint8_t);
  const bool store;
} cmd_t;

void rcommand(uint8_t cmd, uint8_t arg);
void switch_lora(uint8_t sf, uint8_t tx);

#endif