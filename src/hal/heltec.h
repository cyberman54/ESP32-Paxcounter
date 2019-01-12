// clang-format off

#ifndef _HELTEC_H
#define _HELTEC_H

#include <stdint.h>

//#define HAS_BME 0x77  // BME680 sensor on I2C bus (SDI=21/SCL=22); comment out
//if not present

// Hardware related definitions for Heltec LoRa-32 Board
#define HAS_LORA 1 // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED LED_BUILTIN                           // white LED on board
#define HAS_BUTTON KEY_BUILTIN                        // button "PROG" on board

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA OLED_SDA
#define MY_OLED_SCL OLED_SCL
#define MY_OLED_RST OLED_RST

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_IO1 (33)
#define LORA_IO2 LMIC_UNUSED_PIN

#endif
