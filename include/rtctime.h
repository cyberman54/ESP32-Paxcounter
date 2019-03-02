#ifndef _RTCTIME_H
#define _RTCTIME_H

#include "globals.h"
#include "timekeeper.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

extern RtcDS3231<TwoWire> Rtc; // make RTC instance globally available

uint8_t rtc_init(void);
uint8_t set_rtctime(time_t t);
void sync_rtctime(void);
time_t get_rtctime(void);
float get_rtctemp(void);

#endif // _RTCTIME_H