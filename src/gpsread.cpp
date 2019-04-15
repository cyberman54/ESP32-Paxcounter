#if (HAS_GPS)

#include "globals.h"

// Local logging tag
static const char TAG[] = __FILE__;

TinyGPSPlus gps;
gpsStatus_t gps_status;
TaskHandle_t GpsTask;

#ifdef GPS_SERIAL
HardwareSerial GPS_Serial(1); // use UART #1
static uint16_t nmea_txDelay_ms =
    tx_Ticks(NMEA_FRAME_SIZE, GPS_SERIAL) / portTICK_PERIOD_MS;
#else
static uint16_t nmea_txDelay_ms = 0;
#endif

// initialize and configure GPS
int gps_init(void) {

  int ret = 1;

  if (!gps_config()) {
    ESP_LOGE(TAG, "GPS chip initializiation error");
    return 0;
  }

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

// store current GPS location data in struct
void gps_storelocation(gpsStatus_t &gps_store) {
  gps_store.latitude = (int32_t)(gps.location.lat() * 1e6);
  gps_store.longitude = (int32_t)(gps.location.lng() * 1e6);
  gps_store.satellites = (uint8_t)gps.satellites.value();
  gps_store.hdop = (uint16_t)gps.hdop.value();
  gps_store.altitude = (int16_t)gps.altitude.meters();
}

// store current GPS timedate in struct
void IRAM_ATTR gps_storetime(gpsStatus_t &gps_store) {

  gps_store.time_age = gps.time.age();

  if (gps.time.isValid() && gps.date.isValid() && (gps_store.time_age < 1000)) {
    gps_store.timedate.Year =
        CalendarYrToTm(gps.date.year()); // year offset from 1970 in microTime.h
    gps_store.timedate.Month = gps.date.month();
    gps_store.timedate.Day = gps.date.day();
    gps_store.timedate.Hour = gps.time.hour();
    gps_store.timedate.Minute = gps.time.minute();
    gps_store.timedate.Second = gps.time.second();
  } else
    gps_store.timedate = {0};
}

// function to fetch current time from struct; note: this is costly
time_t get_gpstime(gpsStatus_t value) {

  time_t t = timeIsValid(makeTime(value.timedate));

  //  if (t)
  //    t = value.time_age > nmea_txDelay_ms ? t : t - 1;

  // show NMEA data in verbose mode, useful for debugging GPS
  ESP_LOGV(
      TAG,
      "GPS time: %d | GPS NMEA data: passed %d / failed: %d / with fix: %d", t,
      gps.passedChecksum(), gps.failedChecksum(), gps.sentencesWithFix());

  return t;

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

} // gps_loop()

#endif // HAS_GPS