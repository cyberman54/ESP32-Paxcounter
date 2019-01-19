#ifndef _RTC_H
#define _RTC_H

#include "globals.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

int rtc_init(void);
int set_rtc(uint32_t UTCTime);
int set_rtc(RtcDateTime now);
uint32_t get_rtc();
float get_rtc_temp();

#endif