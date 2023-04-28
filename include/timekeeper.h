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

#define HAS_LORA_TIME                                                          \
  ((HAS_LORA) && ((TIME_SYNC_LORASERVER) || (TIME_SYNC_LORAWAN)))
#define HAS_TIME (TIME_SYNC_INTERVAL) && (HAS_LORA_TIME || HAS_GPS)

#define SECS_YR_2000 (946684800UL)    // the time at the start of y2k
#define GPS_UTC_DIFF 315964800UL      // seconds diff between gps and utc epoch
#define LEAP_SECS_SINCE_GPSEPOCH 18UL // state of 2021

enum timesource_t { _gps, _rtc, _lora, _unsynced, _set };

extern const char timeSetSymbols[];
extern Ticker timesyncer;
extern timesource_t timeSource;
extern TaskHandle_t ClockTask;
extern DRAM_ATTR bool TimePulseTick; // 1sec pps flag set by GPS or RTC
#ifdef GPS_INT
extern DRAM_ATTR unsigned long lastPPS;
#endif
#ifdef RTC_INT
extern DRAM_ATTR unsigned long lastRTCpulse;
#endif
extern hw_timer_t *ppsIRQ;

void setTimeSyncIRQ(void);
void time_init(void);
bool timeIsValid(time_t const t);
void calibrateTime(void);
bool setMyTime(uint32_t t_sec, uint16_t t_msec, timesource_t mytimesource);
time_t compileTime(void);
time_t mkgmtime(const struct tm *ptm);
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins);
#endif // _timekeeper_H