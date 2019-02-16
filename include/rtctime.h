#ifndef _RTCTIME_H
#define _RTCTIME_H

#include "globals.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

#ifdef HAS_GPS
#include "gpsread.h"
#endif

extern RtcDS3231<TwoWire> Rtc; // make RTC instance globally available

extern TaskHandle_t ClockTask;
extern hw_timer_t *clockCycle;
extern bool volatile TimePulseTick;

int rtc_init(void);
int set_rtctime(uint32_t t);
int set_rtctime(time_t t);
void sync_rtctime(void);
time_t get_rtctime(void);
float get_rtctemp(void);
void IRAM_ATTR CLOCKIRQ();
int timepulse_init(void);
void timepulse_start();
int sync_TimePulse(void);
int sync_SysTime(time_t);

#endif // _RTCTIME_H