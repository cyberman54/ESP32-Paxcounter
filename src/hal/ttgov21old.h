// clang-format off
// upload_speed 921600
// board esp32dev

#ifndef _TTGOV21OLD_H
#define _TTGOV21OLD_H

#include <stdint.h>


/*  Hardware related definitions for TTGO V2.1 Board
// ATTENTION: check your board version!
// This settings are for boards without label on pcb, or labeled v1.5 on pcb
// see https://github.com/manuelbl/ttn-esp32/wiki/Boards-and-Pins
*/

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC
#define HAS_LED NOT_A_PIN // no usable LED on board
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // rotated display
//#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  LMIC_UNUSED_PIN
// #define LORA_RST  (12)  // v1.5 labelled with pcb date 20180523
#define LORA_IRQ  (26)
#define LORA_IO1  (33)
#define LORA_IO2  (32)

#endif