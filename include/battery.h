#ifndef _BATTERY_H
#define _BATTERY_H

#include <driver/adc.h>
#include <esp_adc_cal.h>

#define DEFAULT_VREF 1100 // tbd: use adc2_vref_to_gpio() for better estimate
#define NO_OF_SAMPLES 64  // we do some multisampling to get better values

uint16_t read_voltage(void);
void calibrate_voltage(void);
bool batt_sufficient(void);

#endif
