// clang-format off

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
#define BOARD_HAS_PSRAM // use extra 4MB external RAM
#define HAS_BUTTON GPIO_NUM_39 // on board button (next to reset)
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board

// GPS settings
#define HAS_GPS 1 // use on board GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M
//#define GPS_INT GPIO_NUM_34 // 30ns accurary timepulse, to be external wired on pcb: NEO 6M Pin#3 -> GPIO34

// Settings for on board DS3231 RTC chip
//#define HAS_RTC MY_OLED_SDA, MY_OLED_SCL // SDA, SCL
//#define RTC_INT GPIO_NUM_13 // timepulse with accuracy +/- 2*e-6 [microseconds] = 0,1728sec / day

// enable only if device has these sensors, otherwise comment these lines
// BME680 sensor on I2C bus
#define HAS_BME SDA, SCL
#define BME_ADDR BME680_I2C_ADDR_PRIMARY // !! connect SDIO of BME680 to GND !!

// display (if connected)
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define MY_OLED_SDA SDA
#define MY_OLED_SCL SCL
#define MY_OLED_RST U8X8_PIN_NONE
//#define DISPLAY_FLIP  1 // use if display is rotated

// Settings for DCF77 interface
#define HAS_DCF77 GPIO_NUM_13

// Settings for IF482 interface
//#define HAS_IF482 9600, SERIAL_7E1, GPIO_NUM_12, GPIO_NUM_14 // IF482 serial port parameters

// user defined sensors (if connected)
//#define HAS_SENSORS 1 // comment out if device has user defined sensors

//#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif
