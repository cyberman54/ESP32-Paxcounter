#ifdef HAS_RTC // we have hardware RTC

#include "rtctime.h"


RtcDS3231<TwoWire> Rtc(Wire); // RTC hardware i2c interface

// initialize RTC
uint8_t rtc_init(void) {
  Wire.begin(HAS_RTC);
  Rtc.Begin(MY_DISPLAY_SDA, MY_DISPLAY_SCL);

  // configure RTC chip
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  if (!Rtc.GetIsRunning()) {
    ESP_LOGI(TAG, "RTC not running, starting now");
    Rtc.SetIsRunning(true);
  }

#if (TIME_SYNC_COMPILEDATE)
  // initialize a blank RTC without battery backup with build time
  RtcDateTime tt = Rtc.GetDateTime();
  time_t t = tt.Epoch32Time(); // sec2000 -> epoch

  if (!Rtc.IsDateTimeValid() || !timeIsValid(t)) {
    ESP_LOGW(TAG, "RTC has no recent time, setting to compiletime");
    Rtc.SetDateTime(RtcDateTime(mkgmtime(compileTime()) -
                                SECS_YR_2000)); // epoch -> sec2000
  }
#endif

  ESP_LOGI(TAG, "RTC initialized");
  return 1; // success

  // failure
  // return 0
} // rtc_init()

uint8_t set_rtctime(time_t t) { // t is sec epoch time
#ifdef RTC_INT // sync rtc 1Hz pulse on top of second
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);  // off
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock); // start
#endif
  Rtc.SetDateTime(RtcDateTime(t - SECS_YR_2000)); // epoch -> sec2000
  ESP_LOGI(TAG, "RTC time synced");
  return 1; // success
} // set_rtctime()

time_t get_rtctime(uint16_t *msec) {
  time_t t = 0;
  *msec = 0;
  if (Rtc.IsDateTimeValid() && Rtc.GetIsRunning()) {
    RtcDateTime tt = Rtc.GetDateTime();
    t = tt.Epoch32Time(); // sec2000 -> epoch
  }

// if we have a RTC pulse, we calculate msec
#ifdef RTC_INT
  uint16_t ppsDiff = millis() - lastRTCpulse;
  if (ppsDiff < 1000)
    *msec = ppsDiff;
  else {
    ESP_LOGD(TAG, "no pulse from RTC");
    return 0;
  }
#endif

  return t;
} // get_rtctime()

float get_rtctemp(void) {
  RtcTemperature temp = Rtc.GetTemperature();
  return temp.AsFloatDegC();
} // get_rtctemp()

#endif // HAS_RTC