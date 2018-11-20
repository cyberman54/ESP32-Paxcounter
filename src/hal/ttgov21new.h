#ifndef _TTGOV21NEW_H
#define _TTGOV21NEW_H

#include <stdint.h>

/*  Hardware related definitions for TTGO V2.1 Board
// ATTENTION: check your board version!
// This settings are for boards labeled v1.6 on pcb, NOT for v1.5 or older
*/

#define HAS_BME 0x77  // BME680 sensor on I2C bus (SDI=21/SCL=22); comment out if not present

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define HAS_LED (25) // green on board LED
#define HAS_BATTERY_PROBE ADC1_GPIO35_CHANNEL // uses GPIO7
#define BATT_FACTOR 2 // voltage divider 100k/100k on board

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

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