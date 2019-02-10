#ifndef _IF482_H
#define _IF482_H

#include "globals.h"
#include "rtctime.h"

int if482_init(void);
void if482_loop(void *pvParameters);
TickType_t tx_time(unsigned long baud, uint32_t config, int8_t rxPin,
                   int8_t txPins);

#endif