#ifndef _SCD30READ_H
#define _SCD30READ_H

#include "globals.h"
#include "SparkFun_SCD30_Arduino_Library.h"
#include <Wire.h>

// #define SDCARD_FILE_HEADER_SCD30     ", Co2, Temp, Humi"  
// for later use

#define SCD30WIRE Wire

bool scd30_init();
void scd30_read();
void scd30_store(scd30Status_t *scd30_store);

#endif // _SCD30READ_H
