#ifndef _POWER_H
#define _POWER_H

#include <Arduino.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>

#include "i2c.h"
#include "reset.h"

#define DEFAULT_VREF 1100 // tbd: use adc2_vref_to_gpio() for better estimate
#define NO_OF_SAMPLES 64  // we do some multisampling to get better values

#ifndef BAT_MAX_VOLTAGE
#define BAT_MAX_VOLTAGE 4200 // millivolts
#endif
#ifndef BAT_MIN_VOLTAGE
#define BAT_MIN_VOLTAGE 3100 // millivolts
#endif

#ifndef PMU_CHG_CUTOFF
#ifdef HAS_PMU
#define PMU_CHG_CUTOFF XPOWERS_CHG_VOL_4V2
#elif defined HAS_IP5306
#define PMU_CHG_CUTOFF 0
#endif
#endif

#ifndef PMU_CHG_CURRENT
#ifdef HAS_PMU
#define PMU_CHG_CURRENT XPOWERS_CHG_CUR_450MA
#elif defined HAS_IP5306
#define PMU_CHG_CURRENT 2
#endif
#endif

#ifdef EXT_POWER_SW
#ifndef EXT_POWER_ON
#define EXT_POWER_ON 1
#endif
#ifndef EXT_POWER_OFF
#define EXT_POWER_OFF (!EXT_POWER_ON)
#endif
#endif

#ifdef BAT_MEASURE_ADC_UNIT // ADC2 wifi bug workaround
extern RTC_NOINIT_ATTR uint64_t RTC_reg_b;
#include "soc/sens_reg.h" // needed for adc pin reset
#endif

typedef uint8_t (*mapFn_t)(uint16_t, uint16_t, uint16_t);

uint16_t read_voltage(void);
void calibrate_voltage(void);
bool batt_sufficient(void);
extern int8_t batt_level;

#ifdef HAS_PMU
#include <XPowersLib.h>
extern XPowersPMU pmu;
enum pmu_power_t { pmu_power_on, pmu_power_off, pmu_power_sleep };
void IRAM_ATTR PMUIRQ();
void AXP192_powerevent_IRQ(void);
void AXP192_power(pmu_power_t powerlevel);
void AXP192_init(void);
void AXP192_showstatus(void);
#endif // HAS_PMU

#ifdef HAS_IP5306
void IP5306_init(void);
void printIP5306Stats(void);
uint8_t IP5306_GetPowerSource(void);
uint8_t IP5306_GetBatteryLevel(void);
uint8_t IP5306_GetBatteryFull(void);
void IP5306_SetChargerEnabled(uint8_t v);
void IP5306_SetChargeCutoffVoltage(uint8_t v);
void IP5306_SetEndChargeCurrentDetection(uint8_t v);
#endif

// The following map functions were taken from
//
// Battery.h - Battery library
// Copyright (c) 2014 Roberto Lo Giacco
// https://github.com/rlogiacco/BatterySense

/**
 * Symmetric sigmoidal approximation
 * https://www.desmos.com/calculator/7m9lu26vpy
 *
 * c - c / (1 + k*x/v)^3
 */
static inline uint8_t sigmoidal(uint16_t voltage, uint16_t minVoltage,
                                uint16_t maxVoltage) {
  // slow
  // uint8_t result = 110 - (110 / (1 + pow(1.468 * (voltage -
  // minVoltage)/(maxVoltage - minVoltage), 6)));

  // steep
  // uint8_t result = 102 - (102 / (1 + pow(1.621 * (voltage -
  // minVoltage)/(maxVoltage - minVoltage), 8.1)));

  // normal
  uint8_t result = 105 - (105 / (1 + pow(1.724 * (voltage - minVoltage) /
                                             (maxVoltage - minVoltage),
                                         5.5)));
  return result >= 100 ? 100 : result;
}

/**
 * Asymmetric sigmoidal approximation
 * https://www.desmos.com/calculator/oyhpsu8jnw
 *
 * c - c / [1 + (k*x/v)^4.5]^3
 */
static inline uint8_t asigmoidal(uint16_t voltage, uint16_t minVoltage,
                                 uint16_t maxVoltage) {
  uint8_t result = 101 - (101 / pow(1 + pow(1.33 * (voltage - minVoltage) /
                                                (maxVoltage - minVoltage),
                                            4.5),
                                    3));
  return result >= 100 ? 100 : result;
}

/**
 * Linear mapping
 * https://www.desmos.com/calculator/sowyhttjta
 *
 * x * 100 / v
 */
static inline uint8_t linear(uint16_t voltage, uint16_t minVoltage,
                             uint16_t maxVoltage) {
  return (unsigned long)(voltage - minVoltage) * 100 /
         (maxVoltage - minVoltage);
}

int8_t read_battlevel(mapFn_t mapFunction = &sigmoidal);

#endif