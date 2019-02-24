#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"
#include "rcommand.h"
#include "spislave.h"
#include "timekeeper.h"
#include <lmic.h>

#ifdef HAS_BME
#include "bme680mems.h"
#endif



void doHousekeeping(void);
uint64_t uptime(void);
void reset_counters(void);
int redirect_log(const char *fmt, va_list args);
uint32_t getFreeRAM();

#endif