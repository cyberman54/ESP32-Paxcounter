#ifndef _timekeeper_H
#define _timekeeper_H

#include "globals.h"
#include "rtctime.h"
#include "irqhandler.h"
#include "timesync.h"
#include "gpsread.h"
#include "if482.h"
#include "dcf77.h"

enum timesource_t { _gps, _rtc, _lora, _unsynced, _set };

extern const char timeSetSymbols[];
extern Ticker timesyncer;
extern timesource_t timeSource;
extern TaskHandle_t ClockTask;
extern Timezone myTZ;
extern bool volatile TimePulseTick; // 1sec pps flag set by GPS or RTC
extern hw_timer_t *ppsIRQ;

void IRAM_ATTR CLOCKIRQ(void);
void clock_init(void);
void clock_loop(void *pvParameters);
void timepulse_start(void);
void setTimeSyncIRQ(void);
uint8_t timepulse_init(void);
time_t timeIsValid(time_t const t);
void calibrateTime(void);
void IRAM_ATTR setMyTime(uint32_t t_sec, uint16_t t_msec,
                         timesource_t mytimesource);
time_t compiledUTC(void);
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins);

#endif // _timekeeper_H