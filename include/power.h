#ifndef _POWER_H
#define _POWER_H

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "i2c.h"
#include "reset.h"

#define DEFAULT_VREF 1100 // tbd: use adc2_vref_to_gpio() for better estimate
#define NO_OF_SAMPLES 64  // we do some multisampling to get better values

uint16_t read_voltage(void);
void calibrate_voltage(void);
bool batt_sufficient(void);

#ifdef HAS_PMU

#include <axp20x.h>
enum pmu_power_t { pmu_power_on, pmu_power_off, pmu_power_sleep };
void AXP192_powerevent_IRQ(void);
void AXP192_power(pmu_power_t powerlevel);
void AXP192_init(void);
void AXP192_showstatus(void);

#endif // HAS_PMU

#endif