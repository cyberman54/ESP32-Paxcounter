#ifndef _BME680_H
#define _BME680_H

#include "globals.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available

void bme_init();
bool bme_read();

#endif