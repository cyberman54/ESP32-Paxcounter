#ifndef _BME280_H
#define _BME280_H

#include "globals.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "../lib/Bosch-BSEC/src/bsec.h"

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available
extern TaskHandle_t Bme280Task;

int bme280_init();
void bme280_loop(void *pvParameters);

#endif