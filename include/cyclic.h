#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"
#include "rcommand.h"
#include "spislave.h"
#include <lmic.h>

#ifdef HAS_BME680
#include "bme680mems.h"
#endif

#ifdef HAS_BME280
#include "bme280.h"
#endif

extern Ticker housekeeper;

void housekeeping(void);
void doHousekeeping(void);
uint64_t uptime(void);
void reset_counters(void);
uint32_t getFreeRAM();

#endif