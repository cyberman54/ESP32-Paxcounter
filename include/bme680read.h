#ifndef _HAS_BME
#define _HAS_BME

#include "globals.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

extern Adafruit_BME680 bme; // Make TinyGPS++ instance globally availabe
extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available
extern TaskHandle_t BmeTask;

void bme_loop(void *pvParameters);

#endif