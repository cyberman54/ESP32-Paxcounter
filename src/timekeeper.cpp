#include "timekeeper.h"
#include "paxcounter.conf"

#if !(HAS_LORA)
#if (TIME_SYNC_LORASERVER)
#error TIME_SYNC_LORASERVER defined, but device has no LORA configured
#elif (TIME_SYNC_LORAWAN)
#error TIME_SYNC_LORAWAN defined, but device has no LORA configured
#endif
#endif

// Local logging tag
static const char TAG[] = __FILE__;

// symbol to display current time source
const char timeSetSymbols[] = {'G', 'R', 'L', '?'};

#ifdef HAS_IF482
HardwareSerial IF482(2); // use UART #2 (#1 may be in use for serial GPS)
#endif

Ticker timesyncer;

void timeSync() { xTaskNotify(irqHandlerTask, TIMESYNC_IRQ, eSetBits); }

void calibrateTime(void) {

  time_t t = 0;
  uint16_t t_msec = 0;

#if (HAS_GPS)
  // fetch recent time from last NMEA record
  t = fetch_gpsTime(&t_msec);
  if (t) {
    timeSource = _gps;
    goto finish;
  }
#endif

// kick off asychronous Lora timeserver timesync if we have
#if (HAS_LORA) && (TIME_SYNC_LORASERVER)
  send_timesync_req();
// kick off asychronous lora network sync if we have
#elif (HAS_LORA) && (TIME_SYNC_LORAWAN)
  LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
#endif

// no time from GPS -> fallback to RTC time while trying lora sync
#ifdef HAS_RTC
  t = get_rtctime();
  if (t) {
    timeSource = _rtc;
    goto finish;
  }
#endif

  goto finish;

finish:

  setMyTime((uint32_t)t, t_msec, timeSource); // set time

} // calibrateTime()

// adjust system time, calibrate RTC and RTC_INT pps
void IRAM_ATTR setMyTime(uint32_t t_sec, uint16_t t_msec,
                         timesource_t mytimesource) {

  // called with invalid timesource?
  if (mytimesource == _unsynced)
    return;

  // increment t_sec only if t_msec > 1000
  time_t time_to_set = (time_t)(t_sec + t_msec / 1000);

  // do we have a valid time?
  if (timeIsValid(time_to_set)) {

    // if we have msec fraction, then wait until top of second with
    // millisecond precision
    if (t_msec % 1000) {
      time_to_set++;
      vTaskDelay(pdMS_TO_TICKS(1000 - t_msec % 1000));
    }

    ESP_LOGD(TAG, "[%0.3f] UTC epoch time: %d.%03d sec", millis() / 1000.0,
             time_to_set, t_msec % 1000);

// if we have got an external timesource, set RTC time and shift RTC_INT pulse
// to top of second
#ifdef HAS_RTC
    if ((mytimesource == _gps) || (mytimesource == _lora))
      set_rtctime(time_to_set);
#endif

// if we have a software pps timer, shift it to top of second
#if (!defined GPS_INT && !defined RTC_INT)
    timerWrite(ppsIRQ, 0); // reset pps timer
    CLOCKIRQ();            // fire clock pps, this advances time 1 sec
#endif

    setTime(time_to_set); // set the time on top of second

    timeSource = mytimesource; // set global variable
    timesyncer.attach(TIME_SYNC_INTERVAL * 60, timeSync);
    ESP_LOGI(TAG, "[%0.3f] Timesync finished, time was set | source: %c",
             millis() / 1000.0, timeSetSymbols[timeSource]);
  } else {
    timesyncer.attach(TIME_SYNC_INTERVAL_RETRY * 60, timeSync);
    ESP_LOGI(TAG, "[%0.3f] Timesync failed, invalid time fetched | source: %c",
             millis() / 1000.0, timeSetSymbols[timeSource]);
  }
}

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

  // start cyclic time sync
  timeSync(); // init systime by RTC or GPS or LORA
  timesyncer.attach(TIME_SYNC_INTERVAL * 60, timeSync);
}

// interrupt service routine triggered by either pps or esp32 hardware timer
void IRAM_ATTR CLOCKIRQ(void) {

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  SyncToPPS(); // advance systime, see microTime.h

// advance wall clock, if we have
#if (defined HAS_IF482 || defined HAS_DCF77)
  xTaskNotifyFromISR(ClockTask, uint32_t(now()), eSetBits,
                     &xHigherPriorityTaskWoken);
#endif

// flip time pulse ticker, if needed
#ifdef HAS_DISPLAY
#if (defined GPS_INT || defined RTC_INT)
  TimePulseTick = !TimePulseTick; // flip pulse ticker
#endif
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

// helper function to calculate serial transmit time
TickType_t tx_Ticks(uint32_t framesize, unsigned long baud, uint32_t config,
                    int8_t rxPin, int8_t txPins) {

  uint32_t databits = ((config & 0x0c) >> 2) + 5;
  uint32_t stopbits = ((config & 0x20) >> 5) + 1;
  uint32_t txTime = (databits + stopbits + 1) * framesize * 1000.0 / baud;
  // +1 for the startbit

  return round(txTime);
}

#if (defined HAS_IF482 || defined HAS_DCF77)

#if (defined HAS_DCF77 && defined HAS_IF482)
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
  // due to concurrent access to i2c bus when reading/writing from/to rtc chip!

#define nextmin(t) (t + DCF77_FRAME_SIZE + 1) // next minute

#ifdef HAS_TWO_LED
  static bool led1_state = false;
#endif
  uint32_t printtime;
  time_t t = *((time_t *)taskparameter), last_printtime = 0; // UTC time seconds

#ifdef HAS_DCF77
  uint8_t *DCFpulse;                  // pointer on array with DCF pulse bits
  DCFpulse = DCF77_Frame(nextmin(t)); // load first DCF frame before start
#elif defined HAS_IF482
  static TickType_t txDelay = pdMS_TO_TICKS(1000 - IF482_SYNC_FIXUP) -
                              tx_Ticks(IF482_FRAME_SIZE, HAS_IF482);
#endif

  // output the next second's pulse/telegram after pps arrived
  for (;;) {

    // wait for timepulse and store UTC time in seconds got
    xTaskNotifyWait(0x00, ULONG_MAX, &printtime, portMAX_DELAY);
    t = time_t(printtime);

    // no confident or no recent time -> suppress clock output
    if ((timeStatus() == timeNotSet) || !(timeIsValid(t)) ||
        (t == last_printtime))
      continue;

#if defined HAS_IF482

    // wait until moment to fire. Normally we won't get notified during this
    // timespan, except when next pps pulse arrives while waiting, because pps
    // was adjusted by recent time sync
    if (xTaskNotifyWait(0x00, ULONG_MAX, &printtime, txDelay) == pdTRUE)
      t = time_t(printtime); // new adjusted UTC time seconds

    // send IF482 telegram
    IF482.print(IF482_Frame(t + 1)); // note: telegram is for *next* second

#elif defined HAS_DCF77

    if (second(t) == DCF77_FRAME_SIZE - 1) // is it time to load new frame?
      DCFpulse = DCF77_Frame(nextmin(t));  // generate frame for next minute

    if (minute(nextmin(t)) ==       // do we still have a recent frame?
        DCFpulse[DCF77_FRAME_SIZE]) // (timepulses could be missed!)
      DCF77_Pulse(t, DCFpulse);     // then output current second's pulse

      // else we have no recent frame, thus suppressing clock output

#endif

// pps blink on secondary LED if we have one
#ifdef HAS_TWO_LED
    if (led1_state)
      switch_LED1(LED_OFF);
    else
      switch_LED1(LED_ON);
    led1_state = !led1_state;
#endif

    last_printtime = t;

  } // for
} // clock_loop()

#endif // HAS_IF482 || defined HAS_DCF77
