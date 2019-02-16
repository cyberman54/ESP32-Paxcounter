#include "rtctime.h"

// Local logging tag
static const char TAG[] = "main";

TaskHandle_t ClockTask;
hw_timer_t *clockCycle = NULL;
bool volatile TimePulseTick = false;

// helper function to setup a pulse per second for time synchronisation
int timepulse_init() {

// use time pulse from GPS as time base with fixed 1Hz frequency
#ifdef GPS_INT

  // setup external interupt for active low RTC INT pin
  pinMode(GPS_INT, INPUT_PULLDOWN);
  // setup external rtc 1Hz clock as pulse per second clock
  ESP_LOGI(TAG, "Time base: external (GPS)");
  return 1; // success

// use pulse from on board RTC chip as time base with fixed frequency
#elif defined RTC_INT

  // setup external interupt for active low RTC INT pin
  pinMode(RTC_INT, INPUT_PULLUP);

  // setup external rtc 1Hz clock as pulse per second clock
  if (I2C_MUTEX_LOCK()) {
    Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
    I2C_MUTEX_UNLOCK();
    ESP_LOGI(TAG, "Time base: external (RTC)");
    return 1; // success
  } else {
    ESP_LOGE(TAG, "I2c bus busy - RTC initialization error");
    return 0; // failure
  }
  return 1; // success

#else
  // use ESP32 hardware timer as time base with adjustable frequency
  clockCycle = timerBegin(1, 8000, true); // set 80 MHz prescaler to 1/10000 sec
  //timerAttachInterrupt(clockCycle, &CLOCKIRQ, true);
  timerAlarmWrite(clockCycle, 10000, true); // 1000ms
  ESP_LOGI(TAG, "Time base: internal (ESP32 hardware timer)");
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

// helper function to sync systime on start of next second
int sync_SysTime(time_t t) {
  if (sync_TimePulse()) {
    setTime(t + 1);
    ESP_LOGD(TAG, "Systime synced on timepulse");
    return 1; // success
  } else
    return 0; // failure
}

// helper function to sync moment on timepulse
int sync_TimePulse(void) {
  // sync on top of next second by timepulse
  if (xSemaphoreTake(TimePulse, pdMS_TO_TICKS(PPS)) == pdTRUE) {
    return 1;
  } // success
  else
    ESP_LOGW(TAG, "Missing timepulse, time not synced");
  return 0; // failure
}

// interrupt service routine triggered by either rtc pps or esp32 hardware
// timer
void IRAM_ATTR CLOCKIRQ() {
  xTaskNotifyFromISR(ClockTask, xTaskGetTickCountFromISR(), eSetBits, NULL);
#if defined GPS_INT || defined RTC_INT
  xSemaphoreGiveFromISR(TimePulse, NULL);
  TimePulseTick = !TimePulseTick; // flip ticker
#endif
  portYIELD_FROM_ISR();
}

#ifdef HAS_RTC // we have hardware RTC

RtcDS3231<TwoWire> Rtc(Wire); // RTC hardware i2c interface

// initialize RTC
int rtc_init(void) {

  // return = 0 -> error / return = 1 -> success

  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {

    Wire.begin(HAS_RTC);
    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) {
      ESP_LOGW(TAG,
               "RTC has no valid RTC date/time, setting to compilation date");
      Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning()) {
      ESP_LOGI(TAG, "RTC not running, starting now");
      Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();

    if (now < compiled) {
      ESP_LOGI(TAG, "RTC date/time is older than compilation date, updating");
      Rtc.SetDateTime(compiled);
    }

    // configure RTC chip
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  } else {
    ESP_LOGE(TAG, "I2c bus busy - RTC initialization error");
    goto error;
  }

  I2C_MUTEX_UNLOCK(); // release i2c bus access
  ESP_LOGI(TAG, "RTC initialized");
  return 1;

error:
  I2C_MUTEX_UNLOCK(); // release i2c bus access
  return 0;

} // rtc_init()

int set_rtctime(time_t t) { // t is seconds epoch time starting 1.1.1970
  if (I2C_MUTEX_LOCK()) {
    Rtc.SetDateTime(RtcDateTime(t));
    I2C_MUTEX_UNLOCK(); // release i2c bus access
    ESP_LOGI(TAG, "RTC calibrated");
    return 1; // success
  }
  return 0; // failure
} // set_rtctime()

int set_rtctime(uint32_t t) { // t is epoch seconds starting 1.1.1970
  return set_rtctime(static_cast<time_t>(t));
  // set_rtctime()
}

time_t get_rtctime(void) {
  // !! never call now() or delay in this function, this would break this
  // function to be used as SyncProvider for Time.h
  time_t t = 0;
  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {
    if (Rtc.IsDateTimeValid()) {
      RtcDateTime tt = Rtc.GetDateTime();
      t = tt.Epoch32Time();
    } else {
      ESP_LOGW(TAG, "RTC has no confident time");
    }
    I2C_MUTEX_UNLOCK(); // release i2c bus access
  }
  return t;
} // get_rtctime()

float get_rtctemp(void) {
  // block i2c bus access
  if (I2C_MUTEX_LOCK()) {
    RtcTemperature temp = Rtc.GetTemperature();
    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return temp.AsFloatDegC();
  } // while
  return 0;
} // get_rtctemp()

#endif // HAS_RTC