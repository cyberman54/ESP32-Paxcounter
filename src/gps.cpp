#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

TinyGPSPlus gps;
gpsStatus_t gps_status;

// read GPS data and cast to global struct
void gps_read() {
  gps_status.latitude = (int32_t)(gps.location.lat() * 1e6);
  gps_status.longitude = (int32_t)(gps.location.lng() * 1e6);
  gps_status.satellites = (uint8_t)gps.satellites.value();
  gps_status.hdop = (uint16_t)gps.hdop.value();
  gps_status.altitude = (int16_t)gps.altitude.meters();
}

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

// initialize and, if needed, configure, GPS
#if defined GPS_SERIAL
  HardwareSerial GPS_Serial(1);
  GPS_Serial.begin(GPS_SERIAL); // serial connect to GPS device

#elif defined GPS_QUECTEL_L76
  Wire.begin(GPS_QUECTEL_L76, 400000); // I2C connect to GPS device with 400 KHz
  uint8_t i2c_ret;
  Wire.beginTransmission(GPS_ADDR);
  Wire.write(0x00);                 // dummy write to start read
  i2c_ret = Wire.endTransmission(); // check if chip is seen on i2c bus

  if (i2c_ret) {
    ESP_LOGE(TAG, "Quectel L76 GPS chip not found on i2c bus, bus error %d",
             i2c_ret);
    return;
  }

#endif

  while (1) {

    if (cfg.gpsmode) {
#if defined GPS_SERIAL

      while (cfg.gpsmode) {
        // feed GPS decoder with serial NMEA data from GPS device
        while (GPS_Serial.available()) {
          gps.encode(GPS_Serial.read());
        }
        vTaskDelay(2 / portTICK_PERIOD_MS); // reset watchdog
      }
      // after GPS function was disabled, close connect to GPS device
      GPS_Serial.end();

#elif defined GPS_QUECTEL_L76

      while (cfg.gpsmode) {
        Wire.requestFrom(GPS_ADDR,
                         128); // 128 is Wire.h buffersize arduino-ESP32
        while (Wire.available()) {
          gps.encode(Wire.read());
          vTaskDelay(2 / portTICK_PERIOD_MS); // delay see L76 datasheet
        }
        vTaskDelay(2 / portTICK_PERIOD_MS); // reset watchdog
      }

#endif // GPS Type
    }

    vTaskDelay(2 / portTICK_PERIOD_MS); // reset watchdog

  } // end of infinite loop

} // gps_loop()

#endif // HAS_GPS