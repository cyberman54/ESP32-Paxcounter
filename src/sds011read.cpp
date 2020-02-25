// routines for fetching data from the SDS011-sensor

// Local logging tag
static const char TAG[] = __FILE__;

#include "sds011read.h"

// UART(2) is unused in this project
#if (HAS_IF482)
#error cannot use IF482 together with SDS011 (both use UART#2)
#endif
static HardwareSerial sdsSerial(2); // so we use it here
static SDS011 sdsSensor;            // fine dust sensor

// the results of the sensor:
float pm25;
float pm10;
boolean isSDS011Active;

// init
bool sds011_init() {
  pm25 = pm10 = 0.0;
  sdsSensor.begin (&sdsSerial,  ESP_PIN_RX, ESP_PIN_TX);
  delay(100);
//  sdsSerial.begin(SDS011_SERIAL);
  //sdsSensor.contmode(0); // for safety: no wakeup/sleep by the sensor
  sds011_sleep();        // we do sleep/wakup by ourselves
  return true;
}

// reading data:
void sds011_loop() {
  if (isSDS011Active) {
    int sdsErrorCode = sdsSensor.read(&pm25, &pm10);
    if (sdsErrorCode) {
      pm25 = pm10 = 0.0;
      ESP_LOGI(TAG, "SDS011 error: %d", sdsErrorCode);
    } else {
      ESP_LOGI(TAG, "fine-dust-values: %5.1f,%4.1f", pm10, pm25);
    }
    sds011_sleep();
  }
  return;
}

// putting the SDS-sensor to sleep
void sds011_sleep(void) {
  sdsSensor.sleep();
  isSDS011Active = false;
}

// start the SDS-sensor
// needs 30 seconds for warming up
void sds011_wakeup() {
  if (!isSDS011Active) {
    sdsSensor.wakeup();
    isSDS011Active = true;
  }
}
