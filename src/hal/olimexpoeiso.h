// clang-format off
// upload_speed 921600
// board esp32-poe-iso

#ifndef _OLIMEXPOEISO_H
#define _OLIMEXPOEISO_H

#include <stdint.h>

// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  2      // this board has an SD-card-reader/writer

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_MEASURE_ADC ADC1_GPIO39_CHANNEL // external power probe GPIO pin
#define BAT_VOLTAGE_DIVIDER 2.1277f // voltage divider 47k/442k on board

#define EXT_POWER_SW GPIO_NUM_12 // switches PoE power, Vext control 0 = off / 1 = on
#define EXT_POWER_ON    1
//#define EXT_POWER_OFF   1

#define HAS_BUTTON KEY_BUILTIN // on board button
#define HAS_LED NOT_A_PIN // no on board LED

#endif
