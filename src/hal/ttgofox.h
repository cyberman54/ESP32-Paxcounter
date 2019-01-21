// clang-format off

#ifndef _TTGOFOX_H
#define _TTGOFOX_H

#include <stdint.h>

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define HAS_LED NOT_A_PIN // green on board LED is useless, is GPIO25, which switches power for Lora+Display
//#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
#define HAS_LOWPOWER_SWITCH GPIO_NUM_25 // switches power for LoRa chip + display (0 = off / 1 = on)
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL
#define BATT_FACTOR 2 // voltage divider 100k/100k on board
#define HAS_BUTTON GPIO_NUM_36 // on board button (next to reset)

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

// Pins for on board DS3231 RTC chip
#define HAS_RTC MY_OLED_SDA, MY_OLED_SCL // SDA, SCL

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  (23)
#define LORA_IRQ  (26)
#define LORA_IO1  (33)
#define LORA_IO2  (32)

#endif