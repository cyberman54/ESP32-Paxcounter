#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = "main";

TinyGPSPlus gps;
gpsStatus_t gps_status;
TaskHandle_t GpsTask;

#ifdef GPS_SERIAL
HardwareSerial GPS_Serial(1);
#endif

// initialize and configure GPS
int gps_init(void) {

  int ret = 1;

#if defined GPS_SERIAL
  GPS_Serial.begin(GPS_SERIAL);
  ESP_LOGI(TAG, "Using serial GPS");
#elif defined GPS_I2C
  Wire.begin(GPS_I2C, 400000); // I2C connect to GPS device with 400 KHz
  Wire.beginTransmission(GPS_ADDR);
  Wire.write(0x00);             // dummy write
  ret = Wire.endTransmission(); // check if chip is seen on i2c bus

  if (ret) {
    ESP_LOGE(TAG,
             "Quectel L76 GPS chip not found on i2c bus, bus error %d. "
             "Stopping GPS-Task.",
             ret);
    ret = 0;
  } else {
    ESP_LOGI(TAG, "Quectel L76 GPS chip found");
  }
#endif

  return ret;
} // gps_init()

// read GPS data and cast to global struct
void gps_read() {
  gps_status.latitude = (int32_t)(gps.location.lat() * 1e6);
  gps_status.longitude = (int32_t)(gps.location.lng() * 1e6);
  gps_status.satellites = (uint8_t)gps.satellites.value();
  gps_status.hdop = (uint16_t)gps.hdop.value();
  gps_status.altitude = (int16_t)gps.altitude.meters();
  // show NMEA data in debug mode, useful for debugging GPS
  ESP_LOGD(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
           gps.passedChecksum(), gps.failedChecksum(), gps.sentencesWithFix());
}

// GPS serial feed FreeRTos Task
void gps_loop(void *pvParameters) {

  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  while (1) {

    if (cfg.payloadmask && GPS_DATA) {
#if defined GPS_SERIAL
      // feed GPS decoder with serial NMEA data from GPS device
      while (GPS_Serial.available()) {
        gps.encode(GPS_Serial.read());
      }
#elif defined GPS_I2C
      Wire.requestFrom(GPS_ADDR, 32); // caution: this is a blocking call
      while (Wire.available()) {
        gps.encode(Wire.read());
        vTaskDelay(2 / portTICK_PERIOD_MS); // 2ms delay according L76 datasheet
      }
#endif
    } // if

    vTaskDelay(2 / portTICK_PERIOD_MS); // yield to CPU

  } // end of infinite loop

  vTaskDelete(NULL); // shoud never be reached

} // gps_loop()

#endif // HAS_GPS