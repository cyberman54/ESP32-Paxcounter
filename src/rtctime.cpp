#include "rtctime.h"

// Local logging tag
static const char TAG[] = "main";

TaskHandle_t ClockTask;
hw_timer_t *clockCycle = NULL;

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

int set_rtctime(time_t t) { // t is epoch time starting 1.1.1970
  if (I2C_MUTEX_LOCK()) {
    Rtc.SetDateTime(RtcDateTime(t));
    I2C_MUTEX_UNLOCK(); // release i2c bus access
    return 1;           // success
  }
  return 0; // failure
} // set_rtctime()

int set_rtctime(uint32_t t) { // t is epoch seconds starting 1.1.1970
  return set_rtctime(static_cast<time_t>(t));
  // set_rtctime()
}

time_t get_rtctime(void) {
  // never call now() in this function, this would cause a recursion!
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

// helper function to setup a pulse for time synchronisation
int pps_init(uint32_t pulse_period_ms) {

// use pulse from on board RTC chip as time base with fixed frequency
#if defined RTC_INT && defined RTC_CLK

  // setup external interupt for active low RTC INT pin
  pinMode(RTC_INT, INPUT_PULLUP);

  // setup external rtc 1Hz clock as pulse per second clock
  ESP_LOGI(TAG, "Time base external clock");
  if (I2C_MUTEX_LOCK()) {
    Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
    I2C_MUTEX_UNLOCK();
  } else {
    ESP_LOGE(TAG, "I2c bus busy - RTC initialization error");
    return 0; // failure
  }
  return 1; // success

  #elif defined RTC_INT && defined HAS_GPS

#else
  // use ESP32 hardware timer as time base with adjustable frequency
  if (pulse_period_ms) {
    ESP_LOGI(TAG, "Time base ESP32 clock");
    clockCycle =
        timerBegin(1, 8000, true); // set 80 MHz prescaler to 1/10000 sec
    timerAttachInterrupt(clockCycle, &CLOCKIRQ, true);
    timerAlarmWrite(clockCycle, 10 * pulse_period_ms, true); // ms
  } else {
    ESP_LOGE(TAG, "Invalid pulse clock frequency");
    return 0; // failure
  }
  return 1; // success
#endif
}

void pps_start() {
#ifdef RTC_INT // start external clock
  //attachInterrupt(digitalPinToInterrupt(RTC_INT), CLOCKIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(RTC_INT), CLOCKIRQ, RISING);
#else // start internal clock
  timerAlarmEnable(clockCycle);
#endif
}

// helper function to sync phase of DCF output signal to start of second t
uint8_t sync_clock(time_t t) {
  time_t tt = t;
  // delay until start of next second
  do {
    tt = now();
  } while (t == tt);
  ESP_LOGI(TAG, "Sync on Sec %d", second(tt));
  return second(tt);
}

// interrupt service routine triggered by either rtc pps or esp32 hardware
// timer
void IRAM_ATTR CLOCKIRQ() {
  xTaskNotifyFromISR(ClockTask, xTaskGetTickCountFromISR(), eSetBits, NULL);
  portYIELD_FROM_ISR();
}