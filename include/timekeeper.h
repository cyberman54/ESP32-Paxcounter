#ifndef _timekeeper_H
#define _timekeeper_H

#include "globals.h"
#include "rtctime.h"
#include "TimeLib.h"
#include "irqhandler.h"
#include "timesync.h"
#include "gpsread.h"
#include "if482.h"
#include "dcf77.h"

extern const char timeSetSymbols[];
extern Ticker timesyncer;

void IRAM_ATTR CLOCKIRQ(void);
void clock_init(void);
void clock_loop(void *pvParameters);
void timepulse_start(void);
void setTimeSyncIRQ(void);
uint8_t timepulse_init(void);
time_t timeIsValid(time_t const t);
void calibrateTime(void);
void IRAM_ATTR setMyTime(uint32_t t_sec, uint16_t t_msec, timesource_t mytimesource);
time_t compiledUTC(void);
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins);

#endif // _timekeeper_H