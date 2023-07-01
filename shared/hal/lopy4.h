// clang-format off
// upload_speed 921600
// board lopy4

#ifndef _LOPY4_H
#define _LOPY4_H

#include <stdint.h>

// Hardware related definitions for Pycom LoPy4 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa

//#defin HAS_SPI 1        // comment out if device shall not send data via SPI
// pin definitions for local wired SPI slave interface
//#define SPI_MOSI  GPIO_NUM_22
//#define SPI_MISO  GPIO_NUM_33
//#define SPI_SCLK  GPIO_NUM_26
//#define SPI_CS    GPIO_NUM_36

#define CFG_sx1276_radio 1
#define HAS_LED NOT_A_PIN // LoPy4 has no on board mono LED, we use on board RGB LED
#define RGB_LED_COUNT 1 // we have 1 LED
#define HAS_RGB_LED FastLED.addLeds<WS2812, GPIO_NUM_0, GRB>(leds, RGB_LED_COUNT);
#define BOARD_HAS_PSRAM // use extra 4MB extern RAM

// Note: Pins for LORA chip SPI interface come from board file pins_arduino.h

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  (21) // pin for switching wifi antenna (P12)
#define WIFI_ANTENNA 0    // 0 = internal, 1 = external

// uncomment defines in this section ONLY if your LoPy lives on a EXPANSION BOARD
//#define HAS_LED (12) // use if LoPy is on Expansion Board, this has a user LED
//#define LED_ACTIVE_LOW 1 // use if LoPy is on Expansion Board, this has a user LED
//#define HAS_BUTTON (13) // user button on expansion board
//#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown
//#define BAT_MEASURE_ADC ADC1_GPIO39_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BAT_VOLTAGE_DIVIDER 2 // voltage divider 1MOhm/1MOhm -> expansion board 3.0
//#define BAT_VOLTAGE_DIVIDER 4 // voltage divider 115kOhm/56kOhm -> expansion board 2.0

#endif