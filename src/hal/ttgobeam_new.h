// clang-format off

#ifndef _TTGOBEAM_H
#define _TTGOBEAM_H

#include <stdint.h>

// Hardware related definitions for TTGO T-Beam board
//
// pinouts taken from http://tinymicros.com/wiki/TTGO_T-Beam

// enable only if device has these sensors, otherwise comment these lines
// BME680 sensor on I2C bus
//#define HAS_BME GPIO_NUM_21, GPIO_NUM_22 // SDA, SCL
//#define BME_ADDR BME680_I2C_ADDR_PRIMARY // connect SDIO of BME680 to GND

#define HAS_LED GPIO_NUM_14 // on board green LED

// user defined sensors
//#define HAS_SENSORS 1 // comment out if device has user defined sensors

//#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define MY_OLED_SDA (21)
//#define MY_OLED_SCL (22)
//#define MY_OLED_RST U8X8_PIN_NONE
//#define DISPLAY_FLIP  1 // use if display is rotated

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC
#define BOARD_HAS_PSRAM // use extra 4MB external RAM

//#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_BUTTON GPIO_NUM_39 // on board button "SW5"

#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board
#define HAS_GPS 1 // use on board GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M or 7M with default configuration

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  (23)
#define LORA_IRQ  (26)
#define LORA_IO1  (33)  // for T-Beam T22_V05 and T22_V07, for other versions use ttgobeam_old.h
#define LORA_IO2  (32)

#endif