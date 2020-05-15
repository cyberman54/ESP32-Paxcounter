// clang-format off
// upload_speed 921600
// board esp32dev

#ifndef _GENERIC_H
#define _GENERIC_H

#include <stdint.h>

// i2c bus definitions
#define MY_DISPLAY_SDA (13)
#define MY_DISPLAY_SCL (16)

// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  1      // this board has an SD-card-reader/writer
// Pins for SD-card
#define SDCARD_CS    (13)
#define SDCARD_MOSI  (15)
#define SDCARD_MISO  (2)
#define SDCARD_SCLK  (14)

// user defined sensors
#define HAS_SENSORS 1 // comment out if device has user defined sensors

//#define BOARD_HAS_PSRAM // use if board has external PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_MEASURE_ADC ADC1_GPIO39_CHANNEL // external power probe GPIO pin
#define BAT_VOLTAGE_DIVIDER 2.1277f // voltage divider 47k/442k on board

#define EXT_POWER_SW GPIO_NUM_12 // switches PoE power, Vext control 0 = off / 1 = on
#define EXT_POWER_ON    1
//#define EXT_POWER_OFF   1

#define HAS_BUTTON (34) // on board button
#define HAS_LED NOT_A_PIN // no on board LED

#endif
