#ifdef HAS_RTC

#include "rtctime.h"

// Local logging tag
static const char TAG[] = "main";

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