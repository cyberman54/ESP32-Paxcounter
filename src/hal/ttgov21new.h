// clang-format off
// upload_speed 921600
// board esp32dev

#ifndef _TTGOV21NEW_H
#define _TTGOV21NEW_H

#include <stdint.h>

/*  Hardware related definitions for TTGO V2.1 Board
// ATTENTION: check your board version!
// This settings are for boards labeled v1.6 on pcb, NOT for v1.5 or older
*/

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

// enable only if device has these sensors, otherwise comment these lines
// BME280 sensor on I2C bus
//#define HAS_BME 1 // Enable BME sensors in general
//#define HAS_BME280 GPIO_NUM_21, GPIO_NUM_22 // SDA, SCL

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define HAS_LED (25) // green on board LED
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

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