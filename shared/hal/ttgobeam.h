// clang-format off
// upload_speed 921600
// board ttgo-t-beam

#ifndef _TTGOBEAM_H
#define _TTGOBEAM_H

#include <stdint.h>

// Hardware related definitions for TTGO T-Beam board
// (only) for older T-Beam version T22_V05 eternal wiring LORA_IO1 to GPIO33 is needed!
//
// pinouts taken from http://tinymicros.com/wiki/TTGO_T-Beam

#define HAS_LED GPIO_NUM_14 // on board green LED, only new version TTGO-BEAM V07
//#define HAS_LED GPIO_NUM_21 // on board green LED, only old verison TTGO-BEAM V05

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC
#define HAS_BUTTON GPIO_NUM_39 // on board button (next to reset)
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

// GPS settings
#define HAS_GPS 1 // use on board GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M
//#define GPS_INT GPIO_NUM_34 // 30ns accurary timepulse, to be external wired on pcb: NEO 6M Pin#3 -> GPIO34

// enable only if device has these sensors, otherwise comment these lines
// BME680 sensor on I2C bus
//#define HAS_BME 1 // Enable BME sensors in general
//#define HAS_BME680 SDA, SCL
//#define BME680_ADDR BME68X_I2C_ADDR_LOW // !! connect SDIO of BME680 to GND !!

// display (if connected)
#define HAS_DISPLAY 1
#define MY_DISPLAY_SDA SDA
#define MY_DISPLAY_SCL SCL
#define MY_DISPLAY_RST NOT_A_PIN
//#define MY_DISPLAY_FLIP  1 // use if display is rotated

//#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif
