#ifndef _BMESENSOR_H
#define _BMESENSOR_H

#include "globals.h"
#include <Wire.h>

#ifdef HAS_BME680
#include "../lib/Bosch-BSEC/src/bsec.h"
#elif defined HAS_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#endif

extern Ticker bmecycler;

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available

// --- Bosch BSEC v1.4.7.4 library configuration ---
// 3,3V supply voltage; 3s max time between sensor_control calls; 4 days
// calibration. Change this const if not applicable for your application (see
// BME680 datasheet)
// Note: 3s max time not exceed BMECYCLE frequency set in paxcounter.conf!
const uint8_t bsec_config_iaq[454] = {
    4,   7,   4,   1,   61,  0,   0,   0,   0,   0,   0,   0,   174, 1,   0,
    0,   48,  0,   1,   0,   0,   192, 168, 71,  64,  49,  119, 76,  0,   0,
    225, 68,  137, 65,  0,   63,  205, 204, 204, 62,  0,   0,   64,  63,  205,
    204, 204, 62,  0,   0,   0,   0,   216, 85,  0,   100, 0,   0,   0,   0,
    0,   0,   0,   0,   28,  0,   2,   0,   0,   244, 1,   225, 0,   25,  0,
    0,   128, 64,  0,   0,   32,  65,  144, 1,   0,   0,   112, 65,  0,   0,
    0,   63,  16,  0,   3,   0,   10,  215, 163, 60,  10,  215, 35,  59,  10,
    215, 35,  59,  9,   0,   5,   0,   0,   0,   0,   0,   1,   88,  0,   9,
    0,   229, 208, 34,  62,  0,   0,   0,   0,   0,   0,   0,   0,   218, 27,
    156, 62,  225, 11,  67,  64,  0,   0,   160, 64,  0,   0,   0,   0,   0,
    0,   0,   0,   94,  75,  72,  189, 93,  254, 159, 64,  66,  62,  160, 191,
    0,   0,   0,   0,   0,   0,   0,   0,   33,  31,  180, 190, 138, 176, 97,
    64,  65,  241, 99,  190, 0,   0,   0,   0,   0,   0,   0,   0,   167, 121,
    71,  61,  165, 189, 41,  192, 184, 30,  189, 64,  12,  0,   10,  0,   0,
    0,   0,   0,   0,   0,   0,   0,   229, 0,   254, 0,   2,   1,   5,   48,
    117, 100, 0,   44,  1,   112, 23,  151, 7,   132, 3,   197, 0,   92,  4,
    144, 1,   64,  1,   64,  1,   144, 1,   48,  117, 48,  117, 48,  117, 48,
    117, 100, 0,   100, 0,   100, 0,   48,  117, 48,  117, 48,  117, 100, 0,
    100, 0,   48,  117, 48,  117, 100, 0,   100, 0,   100, 0,   100, 0,   48,
    117, 48,  117, 48,  117, 100, 0,   100, 0,   100, 0,   48,  117, 48,  117,
    100, 0,   100, 0,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,
    1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,   44,  1,
    44,  1,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,
    7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,   8,   7,
    112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112,
    23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  112, 23,  255, 255,
    255, 255, 255, 255, 255, 255, 220, 5,   220, 5,   220, 5,   255, 255, 255,
    255, 255, 255, 220, 5,   220, 5,   255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 44,  1,   0,   0,   0,   0,
    138, 80,  0,   0};

// Helper functions declarations
int bme_init();
void bmecycle(void);
void bme_storedata(bmeStatus_t *bme_store);
int checkIaqSensorStatus(void);
void loadState(void);
void updateState(void);

#endif