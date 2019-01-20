#ifndef _RTC_H
#define _RTC_H

#include "globals.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

typedef enum {
  useless = 0,     // waiting for good enough signal
  dirty = 1,       // time data available but inconfident
  reserve = 2,     // clock was once synced but now may deviate
  synced_LORA = 3, // clock driven by LORAWAN network
  synced_GPS = 4   // best possible quality, clock is driven by GPS
} clock_state_t;

int rtc_init(void);
int set_rtc(uint32_t UTCTime, clock_state_t state);
int set_rtc(RtcDateTime now, clock_state_t state);
uint32_t get_rtc();
float get_rtc_temp();

#endif