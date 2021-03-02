#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"
#include "rcommand.h"
#include "spislave.h"
#include "mqttclient.h"
#include "bmesensor.h"
#include "display.h"
#include "sds011read.h"
#include "sdcard.h"
#include "macsniff.h"
#include "reset.h"
#ifdef LIBPAX
#include <libpax_api.h>
#endif

extern Ticker cyclicTimer;

void setCyclicIRQ(void);
void doHousekeeping(void);
void reset_counters(void);
uint32_t getFreeRAM();

#endif
