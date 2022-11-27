#include "timekeeper.h"

#if (defined HAS_DCF77 && defined HAS_IF482)
#error You must define at most one of IF482 or DCF77!
#endif


// symbol to display current time source
// G = GPS / R = RTC / L = LORA / * = no sync / ? = never synced
const char timeSetSymbols[] = {'G', 'R', 'L', '*', '?'};

DRAM_ATTR bool TimePulseTick = false;
#ifdef GPS_INT
DRAM_ATTR unsigned long lastPPS = millis();
#endif
#ifdef RTC_INT
DRAM_ATTR unsigned long lastRTCpulse = millis();
#endif

timesource_t timeSource = _unsynced;
TaskHandle_t ClockTask = NULL;
hw_timer_t *ppsIRQ = NULL;

#ifdef HAS_IF482
HardwareSerial IF482(2); // use UART #2 (#1 may be in use for serial GPS)
static TickType_t txDelay = pdMS_TO_TICKS(1000 - IF482_SYNC_FIXUP) -
                            tx_Ticks(IF482_FRAME_SIZE, HAS_IF482);
#if (HAS_SDS011)
#error cannot use IF482 together with SDS011 (both use UART#2)
#endif

#endif // HAS_IF482

Ticker timesyncer;

void setTimeSyncIRQ() { xTaskNotify(irqHandlerTask, TIMESYNC_IRQ, eSetBits); }

#ifdef GPS_INT
// interrupt service routine triggered by GPS PPS
void IRAM_ATTR GPSIRQ(void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // take timestamp
  lastPPS = millis(); // last time of pps

  // yield only if we should
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}
#endif

// interrupt service routine triggered by esp32 hardware timer
void IRAM_ATTR CLOCKIRQ(void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

// advance wall clock, if we have
#if (defined HAS_IF482 || defined HAS_DCF77)
  xTaskNotifyFromISR(ClockTask, uint32_t(time(NULL)), eSetBits,
                     &xHigherPriorityTaskWoken);
#endif

// flip time pulse ticker, if needed
#ifdef HAS_DISPLAY
  TimePulseTick = !TimePulseTick; // flip global variable pulse ticker
#endif

// take timestamp if we have RTC pulse
#ifdef RTC_INT
  lastRTCpulse = millis(); // last time of RTC pulse
#endif

  // yield only if we should
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}

void calibrateTime(void) {
  // kick off asynchronous lora timesync if we have
#if (HAS_LORA_TIME)
  timesync_request();
  if (timeSource == _lora) // did have lora time before?
    return;
#endif

#if ((HAS_GPS) || (HAS_RTC))
  time_t t = 0;
  uint16_t t_msec = 0;

// get GPS time, if we have
#if (HAS_GPS)
  t = get_gpstime(&t_msec);
  if (setMyTime((uint32_t)t, t_msec, _gps))
    return;
#endif

// fallback to RTC time, if we have
#ifdef HAS_RTC
  t = get_rtctime(&t_msec);
  if (setMyTime((uint32_t)t, t_msec, _rtc))
    return;
#endif

#endif
} // calibrateTime()

// set system time (UTC), calibrate RTC and RTC_INT pps
bool setMyTime(uint32_t t_sec, uint16_t t_msec, timesource_t mytimesource) {
  struct timeval tv = {0};

  // called with invalid timesource?
  if (mytimesource == _unsynced)
    return false;

  // increment t_sec if t_msec > 1000
  time_t time_to_set = (time_t)(t_sec + t_msec / 1000);

  // do we have a valid time?
  if (timeIsValid(time_to_set)) {
    // if we have msec fraction, then wait until top of second with
    // millisecond precision
    if (t_msec % 1000) {
      time_to_set++;
      vTaskDelay(pdMS_TO_TICKS(1000 - t_msec % 1000));
    }

    // from here on we are on top of next second

    tv.tv_sec = time_to_set;
    tv.tv_usec = 0;
    sntp_sync_time(&tv);

    ESP_LOGI(TAG, "[%0.3f] UTC time: %d.000 sec", _seconds(), time_to_set);

    // if we have a precise time timesource, set RTC time and shift RTC_INT
    // pulse to top of second
#ifdef HAS_RTC
    if ((mytimesource == _gps) || (mytimesource == _lora))
      set_rtctime(time_to_set);
#endif

    // if we have a software pps timer, shift it to top of second
    if (ppsIRQ != NULL) {
      timerWrite(ppsIRQ, 0); // reset pps timer
      CLOCKIRQ();            // fire clock pps to advance wall clock by 1 sec
    }

    timeSource = mytimesource; // set global variable

    timesyncer.attach(TIME_SYNC_INTERVAL * 60, setTimeSyncIRQ);
    ESP_LOGD(TAG, "[%0.3f] Timesync finished, time was set | timesource=%d",
             _seconds(), mytimesource);
    return true;

  } else {
    timesyncer.attach(TIME_SYNC_INTERVAL_RETRY * 60, setTimeSyncIRQ);
    ESP_LOGV(TAG,
             "[%0.3f] Failed to synchronise time from source %c | unix sec "
             "obtained from source: %d | unix sec at program compilation: %d",
             _seconds(), timeSetSymbols[mytimesource], time_to_set,
             compileTime());
    return false;
  }
}

// helper function to setup a pulse per second for time synchronisation
void timepulse_init(void) {
  // set esp-idf API sntp sync mode
  // sntp_init();
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);

// if we have, use PPS time pulse from GPS for syncing time on top of second
#ifdef GPS_INT
  // setup external interupt pin for rising edge of GPS PPS
  pinMode(GPS_INT, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(GPS_INT), GPSIRQ, RISING);
#endif

// if we have, use pulse from on board RTC chip as time base for calendar time
#if defined RTC_INT
  // setup external rtc 1Hz clock pulse
  Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
  pinMode(RTC_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RTC_INT), CLOCKIRQ, FALLING);
  ESP_LOGI(TAG, "Timepulse: external (RTC)");
#else
  // use ESP32 hardware timer as time base for calendar time
  ppsIRQ = timerBegin(1, 8000, true);   // set 80 MHz prescaler to 1/10000 sec
  timerAlarmWrite(ppsIRQ, 10000, true); // 1000ms
  timerAttachInterrupt(ppsIRQ, &CLOCKIRQ, false);
  timerAlarmEnable(ppsIRQ);
  ESP_LOGI(TAG, "Timepulse: internal (ESP32 hardware timer)");
#endif

  // get time if we don't have one
  if (timeSource != _set) {
    delay(1000);      // wait for first PPS time stamp to arrive
    setTimeSyncIRQ(); // init systime by RTC or GPS or LORA
  }

  // start cyclic time sync
  timesyncer.attach(TIME_SYNC_INTERVAL * 60, setTimeSyncIRQ);
} // timepulse_init

// helper function to check plausibility of a given epoch time
bool timeIsValid(time_t const t) {
  // is t a time in the past? we use compile time to guess
  // compile time is some local time, but we do not know it's time zone
  // thus, we go 1 full day back to be sure to catch a time in the past
  return (t > (compileTime() - 86400));
}

// helper function to calculate serial transmit time
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins) {
  uint32_t databits = ((config & 0x0c) >> 2) + 5;
  uint32_t stopbits = ((config & 0x20) >> 5) + 1;
  uint32_t txTime = (databits + stopbits + 1) * framesize * 1000.0 / baud;
  // +1 for the startbit

  return round(txTime);
}

void clock_loop(void *taskparameter) { // ClockTask
  uint32_t current_time = 0, previous_time = 0;
  time_t tt;
  struct tm t = {0};
#ifdef HAS_TWO_LED
  static bool led1_state = false;
#endif
#ifdef HAS_DCF77
  uint64_t ClockPulse = 0;
  int8_t ClockMinute = -1;
#endif

  // output the next second's pulse/telegram after pps arrived
  for (;;) {
    // wait for timepulse and store UTC time
    xTaskNotifyWait(0x00, ULONG_MAX, &current_time, portMAX_DELAY);

    if ((sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) ||
        !(timeIsValid(current_time)) || (current_time == previous_time))
      continue;

    // set calendar time for next second of clock output
    tt = (time_t)(current_time + 1);
    localtime_r(&tt, &t);
    tt = mktime(&t);

#if defined HAS_IF482

    // wait until moment to fire. Normally we won't get notified during this
    // timespan, except when next pps pulse arrives while waiting, because pps
    // was adjusted by recent time sync, then advance next_time one second
    if (xTaskNotifyWait(0x00, ULONG_MAX, &current_time, txDelay) == pdTRUE) {
      tt = (time_t)(current_time + 1);
      localtime_r(&tt, &t);
      tt = mktime(&t);
    }

    // send IF482 telegram
    IF482.print(IF482_Frame(tt)); // note: telegram is for *next* second

    ESP_LOGD(TAG, "[%0.3f] IF482: %s", _seconds(), IF482_Frame(tt).c_str());

#elif defined HAS_DCF77

    // load new frame if second 59 is reached
    if (t.tm_sec == 0) {
      ClockMinute = t.tm_min;
      t.tm_min++;                  // follow-up minute
      mktime(&t);                  // normalize calendar time
      ClockPulse = DCF77_Frame(t); // generate pulse frame

      /* to do here: leap second handling in second 59 */

      ESP_LOGD(TAG, "[%0.3f] DCF77: new frame for min %d", _seconds(),
               t.tm_min);
    } else {
      // generate impulse
      if (t.tm_min == ClockMinute) { // ensure frame is recent
        DCF77_Pulse(ClockPulse & 1); // output next second
        ClockPulse >>= 1;
      }
    }

#endif

// pps blink on secondary LED if we have one
#ifdef HAS_TWO_LED
    if (led1_state)
      switch_LED1(LED_OFF);
    else
      switch_LED1(LED_ON);
    led1_state = !led1_state;
#endif

    previous_time = current_time;
  } // for
} // clock_loop()

void clock_init(void) {
// setup clock output interface
#ifdef HAS_IF482
  IF482.begin(HAS_IF482);
#elif defined HAS_DCF77
  pinMode(HAS_DCF77, OUTPUT);
#endif

  xTaskCreatePinnedToCore(clock_loop,  // task function
                          "clockloop", // name of task
                          3072,        // stack size of task
                          (void *)1,   // task parameter
                          6,           // priority of the task
                          &ClockTask,  // task handle
                          1);          // CPU core

  _ASSERT(ClockTask != NULL); // has clock task started?
} // clock_init

// we use compile date to create a time_t reference "in the past"
time_t compileTime(void) {
  char s_month[5];
  int year;
  struct tm t = {0};
  static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

  // store compile time once it's calculated
  static time_t secs = -1;

  if (secs == -1) {
    // determine date
    sscanf(__DATE__, "%4s %d %d", s_month, &t.tm_mday, &year);
    t.tm_mon = (strstr(month_names, s_month) - month_names) / 3;
    t.tm_year = year - 1900;
    // determine time
    sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);

    // convert to secs local time
    secs = mktime(&t);
  }

  return secs;
}

static bool IsLeapYear(short year) {
  if (year % 4 != 0)
    return false;
  if (year % 100 != 0)
    return true;
  return (year % 400) == 0;
}

// convert UTC tm time to time_t epoch time
time_t mkgmtime(const struct tm *ptm) {
  const int SecondsPerMinute = 60;
  const int SecondsPerHour = 3600;
  const int SecondsPerDay = 86400;
  const int DaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  time_t secs = 0;
  // tm_year is years since 1900
  int year = ptm->tm_year + 1900;
  for (int y = 1970; y < year; ++y) {
    secs += (IsLeapYear(y) ? 366 : 365) * SecondsPerDay;
  }
  // tm_mon is month from 0..11
  for (int m = 0; m < ptm->tm_mon; ++m) {
    secs += DaysOfMonth[m] * SecondsPerDay;
    if (m == 1 && IsLeapYear(year))
      secs += SecondsPerDay;
  }
  secs += (ptm->tm_mday - 1) * SecondsPerDay;
  secs += ptm->tm_hour * SecondsPerHour;
  secs += ptm->tm_min * SecondsPerMinute;
  secs += ptm->tm_sec;
  return secs;
}

void time_init(void) {
#if (defined HAS_IF482 || defined HAS_DCF77)
  ESP_LOGI(TAG, "Starting clock controller...");
  clock_init();
#endif

#if (HAS_LORA_TIME)
  timesync_init(); // create loraserver time sync task
#endif

  ESP_LOGI(TAG, "Starting time pulse...");
  timepulse_init(); // starts pps and cyclic time sync
}