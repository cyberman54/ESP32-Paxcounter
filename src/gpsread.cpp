#ifdef HAS_GPS

#include "globals.h"
#include <Wire.h>

// Local logging tag
static const char TAG[] = "main";

// read GPS data and cast to global struct
void gps_read() {
  gps_status.latitude = (uint32_t)(gps.location.lat() * 1000000);
  gps_status.longitude = (uint32_t)(gps.location.lng() * 1000000);
  gps_status.satellites = (uint8_t)gps.satellites.value();
  gps_status.hdop = (uint16_t)gps.hdop.value();
  gps_status.altitude = (uint16_t)gps.altitude.meters();
}

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

// initialize and, if needed, configure, GPS
#if defined GPS_SERIAL
  HardwareSerial GPS_Serial(1);
#elif defined GPS_QUECTEL_L76
  Wire.begin(GPS_QUECTEL_L76, 400000); // I2C connect to GPS device with 400 KHz
#endif

  while (1) {

    if (cfg.gpsmode) {
#if defined GPS_SERIAL

      // serial connect to GPS device
      GPS_Serial.begin(GPS_SERIAL);

      while (cfg.gpsmode) {
        // feed GPS decoder with serial NMEA data from GPS device
        while (GPS_Serial.available()) {
          gps.encode(GPS_Serial.read());
        }
        vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog
      }
      // after GPS function was disabled, close connect to GPS device
      GPS_Serial.end();

#elif defined GPS_QUECTEL_L76

      Wire.beginTransmission(GPS_ADDR);
      Wire.write(0x00); // dummy write to start read
      Wire.endTransmission();

      Wire.beginTransmission(GPS_ADDR);
      while (cfg.gpsmode) {
        Wire.requestFrom(GPS_ADDR | 0x01, 32);
        while (Wire.available()) {
          gps.encode(Wire.read());
          vTaskDelay(1 / portTICK_PERIOD_MS); // polling mode: 500ms sleep
        }

        ESP_LOGI(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
                 gps.passedChecksum(), gps.failedChecksum(),
                 gps.sentencesWithFix());
      }
      // after GPS function was disabled, close connect to GPS device

      Wire.endTransmission();

#endif // GPS Type
    }

    vTaskDelay(1 / portTICK_PERIOD_MS); // reset watchdog

  } // end of infinite loop

} // gps_loop()

#endif // HAS_GPS