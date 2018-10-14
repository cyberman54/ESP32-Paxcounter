#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"

void doHousekeeping(void);
uint64_t uptime(void);
void reset_counters(void);
int redirect_log(const char *fmt, va_list args);

#endif