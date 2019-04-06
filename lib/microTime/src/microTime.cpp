/*
  time.c - low level time and date functions
  Copyright (c) Michael Margolis 2009-2014

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  1.0  6  Jan 2010 - initial release
  1.1  12 Feb 2010 - fixed leap year calculation error
  1.2  1  Nov 2010 - fixed setTime bug (thanks to Korman for this)
  1.3  24 Mar 2012 - many edits by Paul Stoffregen: fixed timeStatus() to update
                     status, updated examples for Arduino 1.0, fixed ARM
                     compatibility issues, added TimeArduinoDue and TimeTeensy3
                     examples, add error checking and messages to RTC examples,
                     add examples to DS1307RTC library.
  1.4  5  Sep 2014 - compatibility with Arduino 1.5.7
*/

#include <Arduino.h>

#define TIMELIB_ENABLE_MILLIS
#define usePPS

#include "TimeLib.h"

// Convert days since epoch to week day. Sunday is day 1.
#define DAYS_TO_WDAY(x) (((x) + 4) % 7) + 1

static tmElements_t cacheElements; // a cache of time elements
static time_t cacheTime;           // the time the cache was updated
static uint32_t syncInterval =
    300; // time sync will be attempted after this many seconds

void refreshCache(time_t t) {
  if (t != cacheTime) {
    breakTime(t, cacheElements);
    cacheTime = t;
  }
}

int hour() { // the hour now
  return hour(now());
}

int hour(time_t t) { // the hour for the given time
  refreshCache(t);
  return cacheElements.Hour;
}

int hourFormat12() { // the hour now in 12 hour format
  return hourFormat12(now());
}

int hourFormat12(time_t t) { // the hour for the given time in 12 hour format
  refreshCache(t);
  if (cacheElements.Hour == 0)
    return 12; // 12 midnight
  else if (cacheElements.Hour > 12)
    return cacheElements.Hour - 12;
  else
    return cacheElements.Hour;
}

uint8_t isAM() { // returns true if time now is AM
  return !isPM(now());
}

uint8_t isAM(time_t t) { // returns true if given time is AM
  return !isPM(t);
}

uint8_t isPM() { // returns true if PM
  return isPM(now());
}

uint8_t isPM(time_t t) { // returns true if PM
  return (hour(t) >= 12);
}

int minute() { return minute(now()); }

int minute(time_t t) { // the minute for the given time
  refreshCache(t);
  return cacheElements.Minute;
}

int second() { return second(now()); }

int second(time_t t) { // the second for the given time
  refreshCache(t);
  return cacheElements.Second;
}

int millisecond() {
  uint32_t ms;
  now(ms);
  ms = ms / 1000;
  return (int)ms;
}

int microsecond() {
  uint32_t us;
  now(us);
  return (int)us;
}

int day() { return (day(now())); }

int day(time_t t) { // the day for the given time (0-6)
  refreshCache(t);
  return cacheElements.Day;
}

int weekday() { // Sunday is day 1
  return weekday(now());
}

int weekday(time_t t) {
  refreshCache(t);
  return cacheElements.Wday;
}

int month() { return month(now()); }

int month(time_t t) { // the month for the given time
  refreshCache(t);
  return cacheElements.Month;
}

int year() { // as in Processing, the full four digit year: (2009, 2010 etc)
  return year(now());
}

int year(time_t t) { // the year for the given time
  refreshCache(t);
  return tmYearToCalendar(cacheElements.Year);
}

/*============================================================================*/
/* functions to convert to and from system time */
/* These are for interfacing with time serivces and are not normally needed in a
 * sketch */

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)                                                           \
  (((1970 + (Y)) > 0) && !((1970 + (Y)) % 4) &&                                \
   (((1970 + (Y)) % 100) || !((1970 + (Y)) % 400)))
#define daysInYear(year) ((time_t)(LEAP_YEAR(year) ? 366 : 365))

static const uint8_t monthDays[] = {
    31, 28, 31, 30, 31, 30, 31,
    31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0

void breakTime(time_t time, tmElements_t &tm) {
  // break the given time_t into time components
  // this is a more compact version of the C library localtime function
  // note that year is offset from 1970 !!!

  uint8_t period;
  time_t length;

  tm.Second = time % 60;
  time /= 60; // now it is minutes
  tm.Minute = time % 60;
  time /= 60; // now it is hours
  tm.Hour = time % 24;
  time /= 24; // now it is days since 1 Jan 1970

  // if the number of days since epoch matches cacheTime, then can take date
  // elements from cacheElements and avoid expensive calculation.
  if (time == (cacheTime / SECS_PER_DAY)) {
    if (&tm != &cacheElements) { // check whether tm is actually cacheElements
      tm.Wday = cacheElements.Wday;
      tm.Day = cacheElements.Day;
      tm.Month = cacheElements.Month;
      tm.Year = cacheElements.Year;
    }
    return;
  }

  tm.Wday = DAYS_TO_WDAY(time);

  period = 0;
  while (time >= (length = daysInYear(period))) {
    time -= length;
    period++;
  }
  tm.Year = period; // year is offset from 1970
  // time is now days since 1 Jan of the year

  bool leap_year = LEAP_YEAR(period);
  period = 0;
  while (period < 12 &&
         time >= (length = monthDays[period] + (leap_year && period == 1))) {
    time -= length;
    period++;
  }
  tm.Month = period + 1; // jan is month 1
  // time is now days since the 1st day of the month

  tm.Day = time + 1; // day of month
}

time_t makeTime(const tmElements_t &tm) {
  // assemble time elements into time_t
  // note year argument is offset from 1970 (see macros in time.h to convert to
  // other formats) previous version used full four digit year (or digits since
  // 2000),i.e. 2009 was 2009 or 9

  int i;
  uint32_t seconds;

  // seconds from 1970 till 1 jan 00:00:00 of the given year
  seconds = SECS_PER_DAY * (365 * tm.Year);
  for (i = 0; i < tm.Year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += SECS_PER_DAY; // add extra days for leap years
    }
  }

  // add days for this year, months start from 1
  for (i = 1; i < tm.Month; i++) {
    if ((i == 2) && LEAP_YEAR(tm.Year)) {
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * monthDays[i - 1]; // monthDay array starts from
                                                  // 0
    }
  }
  seconds += (tm.Day - 1) * SECS_PER_DAY;
  seconds += tm.Hour * SECS_PER_HOUR;
  seconds += tm.Minute * SECS_PER_MIN;
  seconds += tm.Second;
  return (time_t)seconds;
}
/*=====================================================*/
/* Low level system time functions  */

static time_t sysTime = 0;
static uint32_t prevMicros = 0;
static time_t nextSyncTime = 0;
static timeStatus_t Status = timeNotSet;

getExternalTime getTimePtr; // pointer to external sync function
// setExternalTime setTimePtr; // not used in this version

#ifdef TIME_DRIFT_INFO      // define this to get drift data
time_t sysUnsyncedTime = 0; // the time sysTime unadjusted by sync
#endif

#ifdef usePPS
void IRAM_ATTR SyncToPPS() {
  sysTime++;
  prevMicros = micros();
}
#endif

time_t now() {
  uint32_t sysTimeMicros;
  return now(sysTimeMicros);
}

time_t now(uint32_t &sysTimeMicros) {
  // calculate number of seconds passed since last call to now()
  while ((sysTimeMicros = micros() - prevMicros) >= 1000000) {
    // micros() and prevMicros are both unsigned ints thus the subtraction will
    // always result in a positive difference. This is OK since it corrects for
    // wrap-around and micros() is monotonic.
    sysTime++;
    prevMicros += 1000000;
#ifdef TIME_DRIFT_INFO
    sysUnsyncedTime++; // this can be compared to the synced time to measure
                       // long term drift
#endif
  }
  if (nextSyncTime <= sysTime) {
    if (getTimePtr != 0) {
      time_t t = getTimePtr();

      if (t != 0) {
        setTime(t);
      } else {
        nextSyncTime = sysTime + syncInterval;
        Status = (Status == timeNotSet) ? timeNotSet : timeNeedsSync;
      }
    }
  }
  return sysTime;
}

void setTime(time_t t) {
#ifdef TIME_DRIFT_INFO
  if (sysUnsyncedTime == 0)
    sysUnsyncedTime = t; // store the time of the first call to set a valid Time
#endif

  sysTime = t;
  nextSyncTime = t + (time_t)syncInterval;
  Status = timeSet;
#ifndef usePPS
  prevMicros =
      micros(); // restart counting from now (thanks to Korman for this fix)
#endif
}

void setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
  // year can be given as full four digit year or two digts (2010 or 10 for
  // 2010); it is converted to years since 1970
  if (yr > 99)
    yr = CalendarYrToTm(yr);
  else
    yr = tmYearToY2k(yr);
  cacheElements.Year = yr;
  cacheElements.Month = mnth;
  cacheElements.Day = dy;
  cacheElements.Hour = hr;
  cacheElements.Minute = min;
  cacheElements.Second = sec;
  cacheTime = makeTime(cacheElements);
  cacheElements.Wday = DAYS_TO_WDAY(cacheTime / SECS_PER_DAY);
  setTime(cacheTime);
}

void adjustTime(long adjustment) { sysTime += adjustment; }

// indicates if time has been set and recently synchronized
timeStatus_t timeStatus() {
  now(); // required to actually update the status
  return Status;
}

void setSyncProvider(getExternalTime getTimeFunction) {
  getTimePtr = getTimeFunction;
  nextSyncTime = sysTime;
  now(); // this will sync the clock
}

void setSyncInterval(
    time_t interval) { // set the number of seconds between re-sync
  syncInterval = (uint32_t)interval;
  nextSyncTime = sysTime + syncInterval;
}
