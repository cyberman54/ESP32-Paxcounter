#ifndef _timekeeper_H
#define _timekeeper_H

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

extern const char timeSetSymbols[];

void IRAM_ATTR CLOCKIRQ(void);
void clock_init(void);
void clock_loop(void *pvParameters);
void time_sync(void);
void timepulse_start(void);
uint8_t syncTime(getExternalTime getTimeFunction, timesource_t const caller);
uint8_t timepulse_init(void);
uint8_t TimeIsValid(time_t const t);
time_t syncProvider_CB(void);
time_t compiledUTC(void);
time_t tmConvert(uint16_t YYYY, uint8_t MM, uint8_t DD, uint8_t hh, uint8_t mm,
                 uint8_t ss);

#endif // _timekeeper_H