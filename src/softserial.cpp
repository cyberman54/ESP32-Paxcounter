// routines for writing data to a second serial port, if present

// Local logging tag
static const char TAG[] = __FILE__;

#include "softserial.h"

#ifdef HAS_SERIAL



bool serial_init() {
  ESP_LOGI(TAG, "Initializing Soft Serial...");
  Serial2.begin(BAUDRATE, SERIAL_8N1, SERIAL_RXD, SERIAL_TXD);
  return true;
}

void serialWriteData(uint16_t noWifi, uint16_t noBle, __attribute__((unused)) uint16_t noBleCWA) {
#if (HAS_SDS011)
  sdsStatus_t sds;
#endif
  ESP_LOGD(TAG, "writing to software serial port");
  Serial2.printf("{\"wifi\": %d, \"ble\": %d ", noWifi, noBle);
#if (COUNT_ENS)
  Serial2.printf(", \"bleCWA\": %d", noBleCWA);
#endif
#if (HAS_SDS011)
  sds011_store(&sds);
  Serial2.printf(", \"pm10\": %5.1f, \"pm10\": %4.1f", sds.pm10, sds.pm25);
#endif
  Serial2.println("}");


}


#endif // (HAS_SERIAL)
