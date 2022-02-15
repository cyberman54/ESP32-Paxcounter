// routines for fetching data from the SDS011-sensor

#if (HAS_SDS011)

// Local logging tag
static const char TAG[] = __FILE__;

#include "sds011read.h"

#if (HAS_IF482)
#error cannot use IF482 together with SDS011 (both use UART#2)
#endif

SdsDustSensor sds(Serial2);

bool isSDS011Active = false;
static float pm10 = 0.0, pm25 = 0.0;

// init
bool sds011_init() {
  sds.begin(9600, SERIAL_8N1, SDS_RX, SDS_TX);
  String version = sds.queryFirmwareVersion().toString();
  ESP_LOGI(TAG, "SDS011 firmware version %s", version);
  sds.setQueryReportingMode();
  sds011_sleep(); // we do sleep/wakup by ourselves

  return true;
}

// reading data:
void sds011_loop() {
  if (isSDS011Active) {
    PmResult pm = sds.queryPm();
    if (!pm.isOk()) {
      ESP_LOGE(TAG, "SDS011 query error %s", pm.statusToString());
      pm10 = pm25 = 0.0;
    } else {
      ESP_LOGI(TAG, "SDS011: %s", pm.toString());
      pm10 = pm.pm10;
      pm25 = pm.pm25;
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
  WorkingStateResult state = sds.sleep();
  isSDS011Active = state.isWorking();
}

// start the SDS-sensor
// needs 30 seconds for warming up
void sds011_wakeup() {
  WorkingStateResult state = sds.wakeup();
  isSDS011Active = state.isWorking();
}

#endif // HAS_SDS011
