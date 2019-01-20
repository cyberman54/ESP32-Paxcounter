#ifdef HAS_RTC

#include "rtctime.h"

// Local logging tag
static const char TAG[] = "main";

RtcDS3231<TwoWire> Rtc(Wire);

clock_state_t RTC_state = useless;

// initialize RTC
int rtc_init() {

  // return = 0 -> error / return = 1 -> success

  // block i2c bus access
  if (xSemaphoreTake(I2Caccess, (DISPLAYREFRESH_MS / portTICK_PERIOD_MS)) ==
      pdTRUE) {

    Wire.begin(HAS_RTC);
    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) {
      ESP_LOGW(TAG,
               "RTC has no valid RTC date/time, setting to compilation date");
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

int set_rtctime(uint32_t UTCTime, clock_state_t state) {
  // return = 0 -> error / return = 1 -> success
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, DISPLAYREFRESH_MS) == pdTRUE) {
#ifdef TIME_SYNC_INTERVAL_RTC
    // shortly stop sync.
    setSyncProvider(NULL);
#endif
    Rtc.SetDateTime(RtcDateTime(UTCTime));
#ifdef TIME_SYNC_INTERVAL_RTC
    // restart sync.
    setSyncProvider(get_rtctime);
#endif
    xSemaphoreGive(I2Caccess); // release i2c bus access
    RTC_state = state;
    return 1;
  }
  return 0;
} // set_rtctime()

int set_rtctime(RtcDateTime now, clock_state_t state) {
  // return = 0 -> error / return = 1 -> success
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, DISPLAYREFRESH_MS) == pdTRUE) {
#ifdef TIME_SYNC_INTERVAL_RTC
    // shortly stop sync.
    setSyncProvider(NULL);
#endif
    Rtc.SetDateTime(now);
#ifdef TIME_SYNC_INTERVAL_RTC
    // restart sync.
    setSyncProvider(get_rtctime);
#endif
    xSemaphoreGive(I2Caccess); // release i2c bus access
    RTC_state = state;
    return 1;
  }
  return 0;
} // set_rtctime()

time_t get_rtctime() {
  time_t rslt = now();
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, DISPLAYREFRESH_MS) == pdTRUE) {
    if (!Rtc.IsDateTimeValid())
      ESP_LOGW(TAG, "RTC lost confidence in the DateTime");
    else
      rslt = (time_t)(Rtc.GetDateTime()).Epoch32Time();
    xSemaphoreGive(I2Caccess); // release i2c bus access
    return rslt;
  }
  return rslt;
} // get_rtc()

void sync_rtctime() {
  time_t t = get_rtctime();
  ESP_LOGI(TAG, "RTC has set system time to %02d/%02d/%d %02d:%02d:%02d",
           month(t), day(t), year(t), hour(t), minute(t), second(t));
#ifdef TIME_SYNC_INTERVAL_RTC
  setSyncInterval((time_t)TIME_SYNC_INTERVAL_RTC);
  //setSyncProvider(get_rtctime); // <<<-- BUG here, causes watchdog timer1 group reboot
  setSyncProvider(NULL); // dummy supressing time sync, to be removed after bug is solved
  if (timeStatus() != timeSet) {
    ESP_LOGE(TAG, "Unable to sync with the RTC");
  } else {
    ESP_LOGI(TAG, "RTC has set the system time");
  }
#endif
} // sync_rtctime;

float get_rtctemp() {
  // block i2c bus access
  while (xSemaphoreTake(I2Caccess, DISPLAYREFRESH_MS) == pdTRUE) {
    RtcTemperature temp = Rtc.GetTemperature();
    xSemaphoreGive(I2Caccess); // release i2c bus access
    return temp.AsFloatDegC();
  } // while
  return 0;
} // get_rtc()

#endif // HAS_RTC