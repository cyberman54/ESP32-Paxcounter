// clang-format off
// upload_speed 921600
// board ttgo-lora32-v1

#ifndef _TTGOV2_H
#define _TTGOV2_H

#include <stdint.h>

// Hardware related definitions for TTGO V2 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED NOT_A_PIN // on-board LED is wired to SCL (used by display) therefore totally useless

// disable brownout detection (needed on TTGOv2 for battery powered operation)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_RST  LMIC_UNUSED_PIN
#define LORA_IO1  (33)
#define LORA_IO2  LMIC_UNUSED_PIN

#endif
