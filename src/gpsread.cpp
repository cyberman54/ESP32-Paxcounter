#ifdef HAS_GPS

#include "globals.h"

// Local logging tag
static const char TAG[] = __FILE__;

TinyGPSPlus gps;
gpsStatus_t gps_status;
TaskHandle_t GpsTask;

#ifdef GPS_SERIAL
HardwareSerial GPS_Serial(1); // use UART #1
TickType_t const gpsDelay_ticks = pdMS_TO_TICKS(1000 - NMEA_BUFFERTIME) -
                                  tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL);
#else
TickType_t const gpsDelay_ticks = pdMS_TO_TICKS(1000 - NMEA_BUFFERTIME);
#endif

// initialize and configure GPS
int gps_init(void) {

  int ret = 1;

  if (!gps_config()) {
    ESP_LOGE(TAG, "GPS chip initializiation error");
    return 0;
  }

// set timeout for reading recent time from GPS
#ifdef GPS_SERIAL // serial GPS

#else // I2C GPS

#endif

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

// detect gps chipset type and configure it with device specific settings
int gps_config() {
  int rslt = 1; // success
#if defined GPS_SERIAL

  /* to come */

#elif defined GPS_I2C

  /* to come */

#endif
  return rslt;
}

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

// function to fetch current time from gps
time_t get_gpstime(void) {

  // set time to wait for arrive next recent NMEA time record
  static const uint32_t gpsDelay_ms = gpsDelay_ticks / portTICK_PERIOD_MS;

  time_t t = 0;

  if ((gps.time.age() < gpsDelay_ms) && (gps.time.isValid()) && (gps.date.isValid())) {

      ESP_LOGD(TAG, "GPS time age: %dms, second: %d, is valid: %s", gps.time.age(), gps.time.second(),
               gps.time.isValid() ? "yes" : "no");

      t = tmConvert(gps.date.year(), gps.date.month(), gps.date.day(),
                    gps.time.hour(), gps.time.minute(), gps.time.second());
    }
  return TimeIsValid(t);
} // get_gpstime()

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
        delay(2); // 2ms delay according L76 datasheet
      }
#endif
    } // if

    delay(2); // yield to CPU

  } // end of infinite loop

  vTaskDelete(GpsTask); // shoud never be reached

} // gps_loop()

#endif // HAS_GPS