#ifndef _BMESENSOR_H
#define _BMESENSOR_H

#if (HAS_BME)

#include <Wire.h>

#include "globals.h"
#include "irqhandler.h"
#include "configmanager.h"

#ifdef HAS_BME680
#include <bsec.h>
#elif defined HAS_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#elif defined HAS_BMP180
#include <Adafruit_BMP085.h>
#elif defined HAS_BMP280
#include <Adafruit_BMP280.h>
#endif

extern Ticker bmecycler;

extern bmeStatus_t
    bme_status; // Make struct for storing gps data globally available

// --- Bosch BSEC library configuration ---
// We use 3,3V supply voltage; 3s max time between sensor_control calls; 4 days
// calibration. Change this const if not applicable for your application (see
// BME680 datasheet) Note: 3s max time must not exceed BMECYCLE frequency set in
// paxcounter.conf!

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

/* Configure the BSEC library with information about the sensor
    18v/33v = Voltage at Vdd. 1.8V or 3.3V
    3s/300s = BSEC operating mode, BSEC_SAMPLE_RATE_LP or BSEC_SAMPLE_RATE_ULP
    4d/28d = Operating age of the sensor in days
    generic_18v_3s_4d
    generic_18v_3s_28d
    generic_18v_300s_4d
    generic_18v_300s_28d
    generic_33v_3s_4d
    generic_33v_3s_28d
    generic_33v_300s_4d
    generic_33v_300s_28d
*/

// Helper functions declarations
int bme_init();
void setBMEIRQ(void);
void bme_storedata(bmeStatus_t *bme_store);
int checkIaqSensorStatus(void);
void loadState(void);
void updateState(void);

#endif

#endif