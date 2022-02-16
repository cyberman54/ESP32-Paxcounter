#ifndef _GPSREAD_H
#define _GPSREAD_H

#include <TinyGPS++.h> // library for parsing NMEA data
#include <RtcDateTime.h>
#include "timekeeper.h"

#ifndef GPS_BAUDRATE
#define GPS_BAUDRATE 115200UL
#endif

extern TinyGPSPlus gps; // Make TinyGPS++ instance globally availabe
extern TaskHandle_t GpsTask;

int gps_init(void);
int gps_config();
bool gps_hasfix();
void gps_storelocation(gpsStatus_t *gps_store);
void gps_loop(void *pvParameters);
time_t get_gpstime(uint16_t *msec);

#endif