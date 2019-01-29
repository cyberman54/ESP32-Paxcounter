#ifndef _RTCTIME_H
#define _RTCTIME_H

#include "globals.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

#ifdef HAS_GPS
#include "gpsread.h"
#endif

extern RtcDS3231<TwoWire> Rtc; // make RTC instance globally available

int rtc_init(void);
int set_rtctime(RtcDateTime t);
int set_rtctime(uint32_t t);
void sync_rtctime(void);
time_t get_rtctime(void);
float get_rtctemp(void);

#endif // _RTCTIME_H