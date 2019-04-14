#ifndef _timekeeper_H
#define _timekeeper_H

#include "globals.h"
#include "rtctime.h"
#include "TimeLib.h"
#include "irqhandler.h"

#if (HAS_GPS)
#include "gpsread.h"
#endif

#ifdef HAS_IF482
#include "if482.h"
#elif defined HAS_DCF77
#include "dcf77.h"
#endif

extern const char timeSetSymbols[];
extern Ticker timesyncer;

void IRAM_ATTR CLOCKIRQ(void);
void clock_init(void);
void clock_loop(void *pvParameters);
void timepulse_start(void);
void timeSync(void);
uint8_t timepulse_init(void);
time_t timeIsValid(time_t const t);
time_t timeProvider(void);
time_t compiledUTC(void);
time_t tmConvert(uint16_t YYYY, uint8_t MM, uint8_t DD, uint8_t hh, uint8_t mm,
                 uint8_t ss);
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins);
time_t TimeSyncAns(uint8_t seqNo, uint64_t unixTime);
void TimeSyncReq(uint8_t seqNo);

#endif // _timekeeper_H