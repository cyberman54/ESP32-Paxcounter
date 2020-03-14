#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"
#include "rcommand.h"
#include "spislave.h"

#if(HAS_LORA)
#include <lmic.h>
#endif

#if (HAS_BME)
#include "bmesensor.h"
#endif

#ifdef HAS_DISPLAY
#include "display.h"
#endif

#if (HAS_SDS011)
#include "sds011read.h"
#endif

#if (HAS_SDCARD)
#include "sdcard.h"
#endif


extern Ticker housekeeper;

void housekeeping(void);
void doHousekeeping(void);
uint64_t uptime(void);
void reset_counters(void);
uint32_t getFreeRAM();

#endif
