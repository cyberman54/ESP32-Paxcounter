#ifndef battery_H
#define battery_H

#include <driver/adc.h>
#include <esp_adc_cal.h>

#define DEFAULT_VREF 1100 // tbd: use adc2_vref_to_gpio() for better estimate
#define NO_OF_SAMPLES 64  // we do multisampling

uint16_t read_voltage(void);

#endif
