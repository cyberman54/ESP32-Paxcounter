#ifndef _timemanager_H
#define _timemanager_H

#include "globals.h"
#include "rtctime.h"

#ifdef HAS_GPS
#include "gpsread.h"
#endif
#ifdef HAS_IF482
#include "if482.h"
#elif defined HAS_DCF77
#include "dcf77.h"
#endif

time_t tmConvert_t(uint16_t YYYY, uint8_t MM, uint8_t DD, uint8_t hh,
                   uint8_t mm, uint8_t ss);
void clock_init(void);
void clock_loop(void *pvParameters);
void time_sync(void);
int wait_for_pulse(void);
int syncTime(time_t);
int syncTime(uint32_t t);
void IRAM_ATTR CLOCKIRQ(void);
int timepulse_init(void);
void timepulse_start(void);
int TimeIsValid(time_t t);
time_t compiledUTC(void);

#endif // _timemanager_H