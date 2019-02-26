#include "timekeeper.h"

// Local logging tag
static const char TAG[] = __FILE__;

// symbol to display current time source
const char timeSetSymbols[] = {'G', 'R', 'L', '?'};

getExternalTime TimeSourcePtr; // pointer to time source function


// syncs systime from external time source and sets/reads RTC, called by
// cyclic.cpp
void timeSync(void) {

  time_t t = 0;

#ifdef HAS_GPS
    // do we have a valid GPS time?
    if (get_gpstime()) { // then let's sync GPS time on top of second

      xSemaphoreTake(TimePulse, pdMS_TO_TICKS(1000)); // wait for pps
      vTaskDelay(gpsDelay_ticks);
      t = get_gpstime(); // fetch time from recent NEMA record
      if (t) {
        t++; // gps time concerns past second, so we add one
        xSemaphoreTake(TimePulse, pdMS_TO_TICKS(1000)); // wait for pps
        setTime(t);
#ifdef HAS_RTC
        set_rtctime(t); // calibrate RTC
#endif
        timeSource = _gps;
        goto exit;
      }
    }
#endif

// no GPS -> fallback to RTC time while trying lora sync
#ifdef HAS_RTC
    t = get_rtctime();
    if (t) {
      setTime(t);
      timeSource = _rtc;
    } else
      ESP_LOGW(TAG, "no confident RTC time");
#endif

// try lora sync if we have
#if defined HAS_LORA && defined TIME_SYNC_LORA
    LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
#endif

exit:

    if (t)
      ESP_LOGD(TAG, "Time was set by %c to %02d:%02d:%02d",
               timeSetSymbols[timeSource], hour(t), minute(t), second(t));
    else
      timeSource = _unsynced;

} // timeSync()

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
time_t TimeIsValid(time_t const t) {
  // is it a time in the past? we use compile date to guess
  return (t >= compiledUTC() ? t : 0);
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

    // no confident time -> suppress clock output
    if (timeStatus() == timeNotSet)
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