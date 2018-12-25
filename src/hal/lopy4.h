// clang-format off

#ifndef _LOPY4_H
#define _LOPY4_H

#include <stdint.h>

// Hardware related definitions for Pycom LoPy4 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa

#define HAS_SPI 1        // comment out if device shall not send data via SPI
// pin definitions for local wired SPI slave interface
#define SPI_MOSI  GPIO_NUM_22
#define SPI_MISO  GPIO_NUM_33
#define SPI_SCLK  GPIO_NUM_26
#define SPI_CS    GPIO_NUM_36

#define CFG_sx1276_radio 1
//#define HAS_LED NOT_A_PIN // LoPy4 has no on board mono LED, we use on board RGB LED
#define HAS_RGB_LED (0)  // WS2812B RGB LED on GPIO0 (P2)
#define BOARD_HAS_PSRAM // use extra 4MB extern RAM

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_SCK 5      // GPIO5  - SX1276 SCK
#define LORA_MISO 19    // GPIO19 - SX1276 MISO
#define LORA_MOSI 27    // GPIO27 - SX1276 MOSI
#define LORA_CS 18      // GPIO18 - SX1276 CS
#define LORA_IRQ 23     // GPIO23 - SX1276 IO0
#define LORA_IO0 LORA_IRQ  // alias
#define LORA_IO1 LORA_IRQ   // tied by diode to IO0
#define LORA_IO2 LORA_IRQ   // tied by diode to IO0
#define LORA_RST NOT_A_PIN

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  (21) // pin for switching wifi antenna (P12)
#define WIFI_ANTENNA 0    // 0 = internal, 1 = external

// uncomment this only if your LoPy runs on a PYTRACK BOARD
//#define HAS_GPS 1
//#define GPS_I2C GPIO_NUM_25, GPIO_NUM_26 // SDA (P22), SCL (P21)
//#define GPS_ADDR 0x10

// uncomment this only if your LoPy runs on a EXPANSION BOARD
#define HAS_LED (12) // use if LoPy is on Expansion Board, this has a user LED
#define LED_ACTIVE_LOW 1 // use if LoPy is on Expansion Board, this has a user LED
#define HAS_BUTTON (13) // user button on expansion board
#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown
#define HAS_BATTERY_PROBE ADC1_GPIO39_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BATT_FACTOR 2 // voltage divider 1MOhm/1MOhm -> expansion board 3.0
//#define BATT_FACTOR 4 // voltage divider 115kOhm/56kOhm -> expansion board 2.0

#endif