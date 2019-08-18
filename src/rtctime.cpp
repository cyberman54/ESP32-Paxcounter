#include "rtctime.h"

// Local logging tag
static const char TAG[] = __FILE__;

#ifdef HAS_RTC // we have hardware RTC

RtcDS3231<TwoWire> Rtc(Wire); // RTC hardware i2c interface

// initialize RTC
uint8_t rtc_init(void) {

  if (I2C_MUTEX_LOCK()) { // block i2c bus access

    Wire.begin(HAS_RTC);
    Rtc.Begin();

    // configure RTC chip
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

    if (!Rtc.GetIsRunning()) {
      ESP_LOGI(TAG, "RTC not running, starting now");
      Rtc.SetIsRunning(true);
    }

#if (TIME_SYNC_COMPILEDATE)
    // initialize a blank RTC without battery backup with compiled time
    RtcDateTime tt = Rtc.GetDateTime();
    time_t t = tt.Epoch32Time(); // sec2000 -> epoch

    if (!Rtc.IsDateTimeValid() || !timeIsValid(t)) {
      ESP_LOGW(TAG, "RTC has no recent time, setting to compiled time");
      Rtc.SetDateTime(
          RtcDateTime(compiledUTC() - SECS_YR_2000)); // epoch -> sec2000
    }
#endif

    I2C_MUTEX_UNLOCK(); // release i2c bus access
    ESP_LOGI(TAG, "RTC initialized");
    return 1; // success
  } else {
    ESP_LOGE(TAG, "RTC initialization error, I2C bus busy");
    return 0; // failure
  }

} // rtc_init()

uint8_t set_rtctime(time_t t) { // t is sec epoch time
  if (I2C_MUTEX_LOCK()) {
#ifdef RTC_INT // sync rtc 1Hz pulse on top of second
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);  // off
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock); // start
#endif
    Rtc.SetDateTime(RtcDateTime(t - SECS_YR_2000)); // epoch -> sec2000
    I2C_MUTEX_UNLOCK();
    ESP_LOGI(TAG, "RTC time synced");
    return 1; // success
  } else {
    ESP_LOGE(TAG, "RTC set time failure");
    return 0; // failure
  }
} // set_rtctime()

time_t get_rtctime(void) {
  time_t t = 0;
  if (I2C_MUTEX_LOCK()) {
    if (Rtc.IsDateTimeValid() && Rtc.GetIsRunning()) {
      RtcDateTime tt = Rtc.GetDateTime();
      t = tt.Epoch32Time(); // sec2000 -> epoch
    }
    I2C_MUTEX_UNLOCK();
    return timeIsValid(t);
  } else {
    ESP_LOGE(TAG, "RTC get time failure");
    return 0; // failure
  }
} // get_rtctime()

float get_rtctemp(void) {
  if (I2C_MUTEX_LOCK()) {
    RtcTemperature temp = Rtc.GetTemperature();
    I2C_MUTEX_UNLOCK();
    return temp.AsFloatDegC();
  }
  return 0;
} // get_rtctemp()

#endif // HAS_RTC