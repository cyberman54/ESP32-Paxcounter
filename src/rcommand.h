#ifndef rcommand_H
#define rcommand_H

#include "senddata.h"
#include "configmanager.h"

void rcommand(uint8_t cmd, uint8_t arg);
void switch_lora(uint8_t sf, uint8_t tx);

#endif