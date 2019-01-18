// clang-format off

#ifndef _HELTECV2_H
#define _HELTECV2_H

#include <stdint.h>

// Hardware related definitions for Heltec V2 LoRa-32 Board

//#define HAS_BME 0x77  // BME680 sensor on I2C bus (SDI=21/SCL=22); comment out
//if not present

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
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
#define LORA_IO1  (35)
#define LORA_IO2  (34)

#endif
