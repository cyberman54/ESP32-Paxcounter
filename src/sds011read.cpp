// routines for fetching data from the SDS011-sensor

#if (HAS_SDS011)

// Local logging tag
static const char TAG[] = __FILE__;

#include "sds011read.h"

#if (HAS_IF482)
#error cannot use IF482 together with SDS011 (both use UART#2)
#endif

// sds011 connected to UART(2)
static SdsDustSensor sdsSensor(Serial2);

// the results of the sensor:
static float pm10, pm25;
bool isSDS011Active = false;

// init
bool sds011_init() {
  pm25 = pm10 = 0.0;
  sdsSensor.begin();

  String version = sdsSensor.queryFirmwareVersion().toString();
  ESP_LOGI(TAG, "SDS011 firmware version %s", version);
  sdsSensor.setQueryReportingMode();
  sds011_sleep(); // we do sleep/wakup by ourselves

  return true;
}

// reading data:
void sds011_loop() {
  if (isSDS011Active) {
    PmResult pm = sdsSensor.queryPm();
    if (!pm.isOk()) {
      pm25 = pm10 = 0.0;
      ESP_LOGE(TAG, "SDS011 query error");
    } else {
      pm25 = pm.pm25;
      pm10 = pm.pm10;
      ESP_LOGI(TAG, "fine-dust-values: %5.1f,%4.1f", pm10, pm25);
    }
    sds011_sleep();
  }
}

// retrieving stored data:
void sds011_store(sdsStatus_t *sds_store) {
  sds_store->pm10 = pm10;
  sds_store->pm25 = pm25;
}

// putting the SDS-sensor to sleep
void sds011_sleep(void) {
  WorkingStateResult state = sdsSensor.sleep();
  isSDS011Active = state.isWorking();
}

// start the SDS-sensor
// needs 30 seconds for warming up
void sds011_wakeup() {
  WorkingStateResult state = sdsSensor.wakeup();
  isSDS011Active = state.isWorking();
}

#endif // HAS_SDS011
