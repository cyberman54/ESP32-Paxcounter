#ifdef HAS_RTC

#include "rtc.h"

// Local logging tag
static const char TAG[] = "main";

RtcDS3231<TwoWire> Rtc(Wire);

clock_state_t RTC_state = useless;

// initialize RTC
int rtc_init() {

  // return = 0 -> error / return = 1 -> success

  // block i2c bus access
  if (xSemaphoreTake(I2Caccess, (2 * DISPLAYREFRESH_MS / portTICK_PERIOD_MS)) ==
      pdTRUE) {

    Wire.begin(HAS_RTC);
    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) {
      ESP_LOGW(TAG, "RTC has no valid RTC date/time, setting to compilation date");
      Rtc.SetDateTime(compiled);
      RTC_state = useless;
    }

    if (!Rtc.GetIsRunning()) {
      ESP_LOGI(TAG, "RTC not running, starting now");
      Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    RTC_state = reserve;

    if (now < compiled) {
      ESP_LOGI(TAG, "RTC date/time is older than compilation date, updating)");
      Rtc.SetDateTime(compiled);
      RTC_state = useless;
    }

    // configure RTC chip
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  } else {
    ESP_LOGE(TAG, "I2c bus busy - RTC initialization error");
    goto error;
  }

  xSemaphoreGive(I2Caccess); // release i2c bus access
  ESP_LOGI(TAG, "RTC initialized");
  return 1;

error:
  xSemaphoreGive(I2Caccess); // release i2c bus access
  return 0;

} // rtc_init()

int set_rtc(uint32_t UTCTime, clock_state_t state) {
  // return = 0 -> error / return = 1 -> success
#ifdef HAS_RTC
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, 2 * DISPLAYREFRESH_MS) == pdTRUE) {
    Rtc.SetDateTime(RtcDateTime(UTCTime));
    xSemaphoreGive(I2Caccess); // release i2c bus access
    RTC_state = state;
    return 1;
  } // while
  return 0;
#endif
} // set_rtc()

int set_rtc(RtcDateTime now, clock_state_t state) {
  // return = 0 -> error / return = 1 -> success
#ifdef HAS_RTC
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, 2 * DISPLAYREFRESH_MS) == pdTRUE) {
    Rtc.SetDateTime(now);
    xSemaphoreGive(I2Caccess); // release i2c bus access
    RTC_state = state;
    return 1;
  } // while
  return 0;
#endif
} // set_rtc()

uint32_t get_rtc() {
#ifdef HAS_RTC
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, 2 * DISPLAYREFRESH_MS) == pdTRUE) {
    if (!Rtc.IsDateTimeValid()) {
      ESP_LOGW(TAG, "RTC lost confidence in the DateTime");
      return 0;
    }
    xSemaphoreGive(I2Caccess); // release i2c bus access
    return Rtc.GetDateTime();
  } // while
  return 0;
#endif
} // get_rtc()

float get_rtc_temp() {
#ifdef HAS_RTC
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, 2 * DISPLAYREFRESH_MS) == pdTRUE) {
    RtcTemperature temp = Rtc.GetTemperature();
    xSemaphoreGive(I2Caccess); // release i2c bus access
    return temp.AsFloatDegC();
  } // while
  return 0;
#endif
} // get_rtc()

#endif // HAS_RTC