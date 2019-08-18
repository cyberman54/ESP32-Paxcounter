#ifndef _GPSREAD_H
#define _GPSREAD_H

#include <TinyGPS++.h> // library for parsing NMEA data
#include <RtcDateTime.h>
#include "timekeeper.h"

#ifdef GPS_I2C // Needed for reading from I2C Bus
#include <Wire.h>
#endif

#define NMEA_FRAME_SIZE 82 // NEMA has a maxium of 82 bytes per record
#define NMEA_COMPENSATION_FACTOR 480 // empiric for Ublox Neo 6M

extern TinyGPSPlus gps; // Make TinyGPS++ instance globally availabe
extern TaskHandle_t GpsTask;

int gps_init(void);
int gps_config();
void gps_storelocation(gpsStatus_t *gps_store);
void gps_loop(void *pvParameters);
time_t fetch_gpsTime(uint16_t *msec);
time_t fetch_gpsTime(void);

#endif