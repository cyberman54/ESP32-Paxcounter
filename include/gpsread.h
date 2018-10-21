#ifndef _GPSREAD_H
#define _GPSREAD_H

#include <TinyGPS++.h> // library for parsing NMEA data
#include <TimeLib.h>

#ifdef GPS_I2C // Needed for reading from I2C Bus
#include <Wire.h>
#endif

typedef struct {
  uint32_t latitude;
  uint32_t longitude;
  uint8_t satellites;
  uint16_t hdop;
  uint16_t altitude;
} gpsStatus_t;

extern TinyGPSPlus gps; // Make TinyGPS++ instance globally availabe
extern gpsStatus_t
    gps_status; // Make struct for storing gps data globally available
extern TaskHandle_t GpsTask;

void gps_read(void);
void gps_loop(void *pvParameters);

#endif