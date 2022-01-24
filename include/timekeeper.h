#ifndef _timekeeper_H
#define _timekeeper_H

#include "globals.h"
#include "rtctime.h"
#include "irqhandler.h"
#include "timesync.h"
#include "gpsread.h"
#include "if482.h"
#include "dcf77.h"
#include "esp_sntp.h"

#define SECS_YR_2000  (946684800UL) // the time at the start of y2k
#define GPS_UTC_DIFF 315964800UL // seconds diff between gps and utc epoch
#define LEAP_SECS_SINCE_GPSEPOCH 18UL // state of 2021

enum timesource_t { _gps, _rtc, _lora, _unsynced, _set };

extern const char timeSetSymbols[];
extern Ticker timesyncer;
extern timesource_t timeSource;
extern TaskHandle_t ClockTask;
extern bool volatile TimePulseTick; // 1sec pps flag set by GPS or RTC
extern hw_timer_t *ppsIRQ;

void IRAM_ATTR CLOCKIRQ(void);
void clock_init(void);
void clock_loop(void *pvParameters);
void timepulse_start(void);
void setTimeSyncIRQ(void);
uint8_t timepulse_init(void);
bool timeIsValid(time_t const t);
void calibrateTime(void);
void IRAM_ATTR setMyTime(uint32_t t_sec, uint16_t t_msec,
                         timesource_t mytimesource);
time_t compileTime(const String compile_date);
time_t mkgmtime(const struct tm *ptm);
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins);

#endif // _timekeeper_H