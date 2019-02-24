#include "timekeeper.h"

// Local logging tag
static const char TAG[] = "main";

void time_sync() {
  // synchonization of systime with external time source (GPS/LORA)
  // frequently called from cyclic.cpp

#ifdef TIME_SYNC_INTERVAL

  if (timeStatus() == timeSet)
    return;

#ifdef HAS_GPS
  if (syncTime(get_gpstime(), _gps))
    return; // attempt sync with GPS time
#endif

// no GPS -> fallback to RTC time while trying lora sync
#ifdef HAS_RTC
  if (!syncTime(get_rtctime(), _rtc)) // sync with RTC time
    ESP_LOGW(TAG, "no confident RTC time");
#endif

// try lora sync if we have
#if defined HAS_LORA && defined TIME_SYNC_LORA
  LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
#endif

#endif // TIME_SYNC_INTERVAL
} // time_sync()

// helper function to sync time on start of next second
uint8_t syncTime(time_t const t, uint8_t const caller) {

  // symbol to display current time source
  const char timeSetSymbols[] = {'G', 'R', 'L', '?'};

  if (TimeIsValid(t)) {
    uint8_t const TimeIsPulseSynced =
        wait_for_pulse(); // wait for next 1pps timepulse
    setTime(t);           // sync time and reset timeStatus() to timeSet
    adjustTime(1);        // forward time to next second
    timeSource = timeSetSymbols[caller];
    ESP_LOGD(TAG, "Time source %c set time to %02d:%02d:%02d", timeSource,
             hour(t), minute(t), second(t));
#ifdef HAS_RTC
    if ((TimeIsPulseSynced) && (caller != _rtc))
      set_rtctime(now());
#endif
    return 1; // success

  } else {
    ESP_LOGD(TAG, "Time source %c sync attempt failed", timeSetSymbols[caller]);
    timeSource = timeSetSymbols[_unsynced];
    return 0; // failure
  }
} // syncTime()

// helper function to sync moment on timepulse
uint8_t wait_for_pulse(void) {
  // sync on top of next second with 1pps timepulse
  if (xSemaphoreTake(TimePulse, pdMS_TO_TICKS(1010)) == pdTRUE)
    return 1; // success
  ESP_LOGD(TAG, "Missing timepulse");
  return 0; // failure
}

// helper function to setup a pulse per second for time synchronisation
uint8_t timepulse_init() {

// use time pulse from GPS as time base with fixed 1Hz frequency
#ifdef GPS_INT

  // setup external interupt pin for GPS INT output
  pinMode(GPS_INT, INPUT_PULLDOWN);
  // setup external rtc 1Hz clock as pulse per second clock
  ESP_LOGI(TAG, "Timepulse: external (GPS)");
  return 1; // success

// use pulse from on board RTC chip as time base with fixed frequency
#elif defined RTC_INT

  // setup external interupt pin for active low RTC INT output
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
  clockCycle = timerBegin(1, 8000, true); // set 80 MHz prescaler to 1/10000 sec
  timerAlarmWrite(clockCycle, 10000, true); // 1000ms
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
  timerAttachInterrupt(clockCycle, &CLOCKIRQ, true);
  timerAlarmEnable(clockCycle);
#endif
}

// interrupt service routine triggered by either pps or esp32 hardware timer
void IRAM_ATTR CLOCKIRQ(void) {
  if (ClockTask != NULL)
    xTaskNotifyFromISR(ClockTask, xTaskGetTickCountFromISR(), eSetBits, NULL);
#if defined GPS_INT || defined RTC_INT
  xSemaphoreGiveFromISR(TimePulse, NULL);
  TimePulseTick = !TimePulseTick; // flip ticker
#endif
  portYIELD_FROM_ISR();
}

// helper function to check plausibility of a time
uint8_t TimeIsValid(time_t const t) {
  // is it a time in the past? we use compile date to guess
  ESP_LOGD(TAG, "t=%d, tt=%d, valid: %s", t, compiledUTC(),
           (t >= compiledUTC()) ? "yes" : "no");
  return (t >= compiledUTC());
}

// helper function to convert compile time to UTC time
time_t compiledUTC(void) {
  time_t t = RtcDateTime(__DATE__, __TIME__).Epoch32Time();
  return myTZ.toUTC(t);
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

  xTaskCreatePinnedToCore(clock_loop,  // task function
                          "clockloop", // name of task
                          2048,        // stack size of task
                          (void *)1,   // task parameter
                          4,           // priority of the task
                          &ClockTask,  // task handle
                          0);          // CPU core

  assert(ClockTask); // has clock task started?
} // clock_init

void clock_loop(void *pvParameters) { // ClockTask

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  TickType_t wakeTime;
  time_t t;

#define t1(t) (t + DCF77_FRAME_SIZE + 1) // future time for next DCF77 frame
#define t2(t) (t + 1) // future time for sync with 1pps trigger

  // preload first DCF frame before start
#ifdef HAS_DCF77
  uint8_t *DCFpulse; // pointer on array with DCF pulse bits
  DCFpulse = DCF77_Frame(t1(now()));
#endif

  // output time telegram for second following sec beginning with timepulse
  for (;;) {
    xTaskNotifyWait(0x00, ULONG_MAX, &wakeTime,
                    portMAX_DELAY); // wait for timepulse

    if (timeStatus() != timeSet) // no confident time -> no output to clock
      continue;

    t = now(); // payload to send to clock

#if defined HAS_IF482

    IF482_Pulse(t2(t)); // next second

#elif defined HAS_DCF77

    if (second(t) == DCF77_FRAME_SIZE - 1) // is it time to load new frame?
      DCFpulse = DCF77_Frame(t1(t));       // generate next frame

    if (DCFpulse[DCF77_FRAME_SIZE] ==
        minute(t1(t))) // have recent frame? (pulses could be missed!)
      DCF77_Pulse(t2(t), DCFpulse); // then output next second of this frame

#endif

  } // for
} // clock_loop()

#endif // HAS_IF482 || defined HAS_DCF77