#ifndef _CORONA_h
#define _CORONA_H

// inspired by https://github.com/kmetz/BLEExposureNotificationBeeper
// (c) by Kaspar Metz
// modified for use in the Paxcounter by AQ

#include "globals.h"
#include <map>

bool cwa_init(void);
bool cwa_mac_add(uint16_t hashedmac);
void cwa_clear(void);
uint16_t cwa_report(void);

#endif
