#include "timekeeper.h"

// Local logging tag
static const char TAG[] = __FILE__;

// symbol to display current time source
const char timeSetSymbols[] = {'G', 'R', 'L', '?'};

Ticker timesyncer;

void timeSync() { xTaskNotify(irqHandlerTask, TIMESYNC_IRQ, eSetBits); }

time_t timeProvider(void) {

  time_t t = 0;

#ifdef HAS_GPS
  t = get_gpstime(); // fetch recent time from last NEMA record
  if (t) {
#ifdef HAS_RTC
    set_rtctime(t); // calibrate RTC
#endif
    timeSource = _gps;
    timesyncer.attach(TIME_SYNC_INTERVAL * 60, timeSync); // regular repeat
    return t;
  }
#endif

// no GPS -> fallback to RTC time while trying lora sync
#ifdef HAS_RTC
  t = get_rtctime();
  if (t) {
    timeSource = _rtc;
    timesyncer.attach(60, timeSync); // short retry
  }
#endif

// kick off asychronous DB timesync if we have
#if(DBTIMESYNC)
  send_DBtime_req();
// kick off asychronous lora sync if we have
#elif defined HAS_LORA && (TIME_SYNC_LORA)
  LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
#endif

  if (!t) {
    timeSource = _unsynced;
    timesyncer.attach(60, timeSync); // short retry
  }

  return t;

} // timeProvider()

// helper function to setup a pulse per second for time synchronisation
uint8_t timepulse_init() {

// use time pulse from GPS as time base with fixed 1Hz frequency
#ifdef GPS_INT

  // setup external interupt pin for rising edge GPS INT
  pinMode(GPS_INT, INPUT_PULLDOWN);
  // setup external rtc 1Hz clock as pulse per second clock
  ESP_LOGI(TAG, "Timepulse: external (GPS)");
  return 1; // success

// use pulse from on board RTC chip as time base with fixed frequency
#elif defined RTC_INT

  // setup external interupt pin for falling edge RTC INT
  pinMode(RTC_INT, INPUT_PULLUP);

  // setup external rtc 1Hz clock as pulse per second clock
  if (I2C_MUTEX_LOCK()) {
    Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
    I2C_MUTEX_UNLOCK();
    ESP_LOGI(TAG, "Timepulse: external (RTC)");
    return 1; // success
  } else {
    ESP_LOGE(TAG, "RTC initialization error, I2C bus busy");
    return 0; // failure
  }
  return 1; // success

#else
  // use ESP32 hardware timer as time base with adjustable frequency
  ppsIRQ = timerBegin(1, 8000, true);   // set 80 MHz prescaler to 1/10000 sec
  timerAlarmWrite(ppsIRQ, 10000, true); // 1000ms
  ESP_LOGI(TAG, "Timepulse: internal (ESP32 hardware timer)");
  return 1; // success

#endif
} // timepulse_init

void timepulse_start(void) {
#ifdef GPS_INT // start external clock gps pps line
  attachInterrupt(digitalPinToInterrupt(GPS_INT), CLOCKIRQ, RISING);
#elif defined RTC_INT // start external clock rtc
  attachInterrupt(digitalPinToInterrupt(RTC_INT), CLOCKIRQ, FALLING);
#else                 // start internal clock esp32 hardware timer
  timerAttachInterrupt(ppsIRQ, &CLOCKIRQ, true);
  timerAlarmEnable(ppsIRQ);
#endif
}

// interrupt service routine triggered by either pps or esp32 hardware timer
void IRAM_ATTR CLOCKIRQ(void) {

  BaseType_t xHigherPriorityTaskWoken;
  SyncToPPS(); // calibrates UTC systime, see Time.h
  xHigherPriorityTaskWoken = pdFALSE;

  if (ClockTask != NULL)
    xTaskNotifyFromISR(ClockTask, uint32_t(now()), eSetBits,
                       &xHigherPriorityTaskWoken);

#if defined GPS_INT || defined RTC_INT
  xSemaphoreGiveFromISR(TimePulse, &xHigherPriorityTaskWoken);
  TimePulseTick = !TimePulseTick; // flip ticker
#endif

  // yield only if we should
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}

// helper function to check plausibility of a time
time_t timeIsValid(time_t const t) {
  // is it a time in the past? we use compile date to guess
  return (t >= compiledUTC() ? t : 0);
}

// helper function to convert compile time to UTC time
time_t compiledUTC(void) {
  static time_t t = myTZ.toUTC(RtcDateTime(__DATE__, __TIME__).Epoch32Time());
  return t;
}

// helper function to convert gps date/time into time_t
time_t tmConvert(uint16_t YYYY, uint8_t MM, uint8_t DD, uint8_t hh, uint8_t mm,
                 uint8_t ss) {
  tmElements_t tm;
  tm.Year = CalendarYrToTm(YYYY); // year offset from 1970 in time.h
  tm.Month = MM;
  tm.Day = DD;
  tm.Hour = hh;
  tm.Minute = mm;
  tm.Second = ss;
  return makeTime(tm);
}

// helper function to calculate serial transmit time
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins) {

  uint32_t databits = ((config & 0x0c) >> 2) + 5;
  uint32_t stopbits = ((config & 0x20) >> 5) + 1;
  uint32_t txTime = (databits + stopbits + 2) * framesize * 1000.0 / baud;
  // +1 ms margin for the startbit +1 ms for pending processing time

  return round(txTime);
}

#if defined HAS_IF482 || defined HAS_DCF77

#if defined HAS_DCF77 && defined HAS_IF482
#error You must define at most one of IF482 or DCF77!
#endif

void clock_init(void) {

// setup clock output interface
#ifdef HAS_IF482
  IF482.begin(HAS_IF482);
#elif defined HAS_DCF77
  pinMode(HAS_DCF77, OUTPUT);
#endif

  userUTCTime = now();

  xTaskCreatePinnedToCore(clock_loop,           // task function
                          "clockloop",          // name of task
                          2048,                 // stack size of task
                          (void *)&userUTCTime, // start time as task parameter
                          4,                    // priority of the task
                          &ClockTask,           // task handle
                          1);                   // CPU core

  assert(ClockTask); // has clock task started?
} // clock_init

void clock_loop(void *taskparameter) { // ClockTask

  // caveat: don't use now() in this task, it will cause a race condition
  // due to concurrent access to i2c bus for setting rtc via SyncProvider!

#define nextmin(t) (t + DCF77_FRAME_SIZE + 1) // next minute

  uint32_t printtime;
  time_t t = *((time_t *)taskparameter); // UTC time seconds

  // preload first DCF frame before start
#ifdef HAS_DCF77
  uint8_t *DCFpulse; // pointer on array with DCF pulse bits
  DCFpulse = DCF77_Frame(nextmin(t));
#endif

  // output the next second's pulse after timepulse arrived
  for (;;) {
    xTaskNotifyWait(0x00, ULONG_MAX, &printtime,
                    portMAX_DELAY); // wait for timepulse

    // no confident time -> we suppress clock output
    if (timeStatus() == timeNotSet)
      continue;

    t = time_t(printtime); // UTC time seconds

#if defined HAS_IF482

    IF482_Pulse(t);

#elif defined HAS_DCF77

    if (second(t) == DCF77_FRAME_SIZE - 1) // is it time to load new frame?
      DCFpulse = DCF77_Frame(nextmin(t));  // generate frame for next minute

    if (minute(nextmin(t)) ==       // do we still have a recent frame?
        DCFpulse[DCF77_FRAME_SIZE]) // (timepulses could be missed!)
      DCF77_Pulse(t, DCFpulse);     // then output current second's pulse
    else
      continue; // no recent frame -> we suppress clock output

#endif

  } // for
} // clock_loop()

#endif // HAS_IF482 || defined HAS_DCF77