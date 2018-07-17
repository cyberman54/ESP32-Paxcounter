#ifndef gps_H
#define gps_H

#include <TinyGPS++.h>

extern TinyGPSPlus gps;        // Make TinyGPS++ instance globally availabe

void gps_read(void);
void gps_loop(void *pvParameters);

#endif