#ifndef _GENERIC_H
#define _GENERIC_H

#include <stdint.h>

// Hardware related definitions for generic ESP32 boards

#define HAS_LORA 1 // comment out if device shall not send data via LoRa or has no LoRa
#define HAS_SPI 1  // comment out if device shall not send data via SPI

#define CFG_sx1276_radio 1 // select LoRa chip
//#define CFG_sx1272_radio 1 // select LoRa chip
#define BOARD_HAS_PSRAM // use if board has external PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // use if display is rotated
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // uses GPIO7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board

#define HAS_LED (21) // on board  LED
#define HAS_BUTTON (39) // on board button
#define HAS_RGB_LED (0) // WS2812B RGB LED on GPIO0

#define BOARD_HAS_PSRAM // use extra 4MB extern RAM

#define HAS_GPS 1 // use if board has GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15 // UBlox NEO 6M or 7M with default configuration

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (4)
#define MY_OLED_SCL (15)
#define MY_OLED_RST (16)

// Pins for LORA chip reset and interrupt lines
#define RST   (14)
#define DIO0  (26)
#define DIO1  (33)
#define DIO2  LMIC_UNUSED_PIN

// I2C config for Microchip 24AA02E64 DEVEUI unique address
#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits

#endif