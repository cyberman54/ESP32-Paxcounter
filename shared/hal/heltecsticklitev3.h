// clang-format off
// upload_speed 921600
// board heltec_wifi_lora_32_V3

#ifndef _HELTECSTICKLITEV3_H
#define _HELTECSTICKLITEV3_H

#include <stdint.h>

// Hardware related definitions for Heltec V3 LoRa-32 S3 Board
// see https://docs.heltec.org/en/node/esp32/wireless_stick_lite/index.html

// LORA settings
#define HAS_LORA 1 // use on board LORA
#define CFG_sx1262_radio 1 // HPD16A
// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_IRQ DIO0
#define LORA_IO1 LMIC_UNUSED_PIN
#define LORA_IO2 LMIC_UNUSED_PIN
#define LORA_SCK SCK
#define LORA_MISO MISO
#define LORA_MOSI MOSI
#define LORA_RST RST_LoRa
#define LORA_BUSY BUSY_LoRa
#define LORA_CS SS

#define HAS_LED LED_BUILTIN           
#define HAS_BUTTON 0

#define BAT_MEASURE_ADC ADC1_CHANNEL_0 // battery probe pin is GPIO1
#define BAT_VOLTAGE_DIVIDER 4 // voltage divider 100k/390k on board

// switches battery power and Vext, switch logic 0 = on / 1 = off
#define EXT_POWER_SW Vext
#define EXT_POWER_ON 0

#endif