#ifndef gps_H
#define gps_H

#include <TinyGPS++.h>

void gps_read(void);
void gps_loop(void *pvParameters);

#endif