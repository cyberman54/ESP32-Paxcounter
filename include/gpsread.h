#ifndef _GPSREAD_H
#define _GPSREAD_H

#include <TinyGPS++.h> // library for parsing NMEA data

#ifdef GPS_I2C // Needed for reading from I2C Bus
#include <Wire.h>
#endif

extern TinyGPSPlus gps; // Make TinyGPS++ instance globally availabe
extern gpsStatus_t
    gps_status; // Make struct for storing gps data globally available
extern TaskHandle_t GpsTask;

int gps_init(void);
void gps_read(void);
void gps_loop(void *pvParameters);

#endif