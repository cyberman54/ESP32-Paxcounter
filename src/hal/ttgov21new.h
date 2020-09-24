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

#define HAS_LORA         1 // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // HPD13A LoRa SoC

// enable only if you want to store a local paxcount table on the device
#define HAS_SDCARD  1      // this board has an SD-card-reader/writer
// Pins for SD-card
#define SDCARD_CS    (13)
#define SDCARD_MOSI  (15)
#define SDCARD_MISO  (2)
#define SDCARD_SCLK  (14)

#define HAS_DISPLAY 1
#define HAS_LED (25) // green on board LED
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 100k/100k on board

// Pins for I2C interface of OLED Display
#define MY_DISPLAY_SDA (21)
#define MY_DISPLAY_SCL (22)
#define MY_DISPLAY_RST NOT_A_PIN

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
