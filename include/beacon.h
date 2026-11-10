#ifndef _BEACON_H
#define _BEACON_H

#include "globals.h"

// initialize beacon module, starts beacon transmitter if enabled in cfg
void beacon_init(void);

// start / stop BLE iBeacon advertising
void beacon_start(void);
void beacon_stop(void);

// apply changed beacon major/minor number to a running beacon
void beacon_update(void);

// true while beacon transmitter is active
bool beacon_isrunning(void);

#endif
