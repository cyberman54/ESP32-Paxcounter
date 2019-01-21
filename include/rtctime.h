#ifndef _RTCTIME_H
#define _RTCTIME_H

#include "globals.h"
#include <Time.h>
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

#ifdef HAS_GPS
#include "gpsread.h"
#endif

typedef enum {
  useless = 0,     // waiting for good enough signal
  dirty = 1,       // time data available but inconfident
  reserve = 2,     // clock was once synced but now may deviate
  synced_LORA = 3, // clock driven by LORAWAN network
  synced_GPS = 4   // best possible quality, clock is driven by GPS
} clock_state_t;

int rtc_init(void);
void sync_rtctime(void);
int set_rtctime(uint32_t UTCTime);
int set_rtctime(RtcDateTime now);
void sync_rtctime(void);
time_t get_rtctime(void);
float get_rtctemp(void);
time_t gpsTimeSync(void);

#endif // _RTCTIME_H