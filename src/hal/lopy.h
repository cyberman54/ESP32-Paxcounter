#ifndef _LOPY_H
#define _LOPY_H

#include <stdint.h>

// Hardware related definitions for Pycom LoPy Board (NOT LoPy4)

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1272_radio 1
#define HAS_LED NOT_A_PIN // LoPy4 has no on board mono LED, we use on board RGB LED
#define HAS_RGB_LED (0)  // WS2812B RGB LED on GPIO0

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5)  // GPIO5  - SX1276 SCK
#define LORA_CS   (17) // GPIO17 - SX1276 CS
#define LORA_MISO (19) // GPIO19 - SX1276 MISO
#define LORA_MOSI (27) // GPIO27 - SX1276 MOSI
#define LORA_RST  (18) // GPIO18 - SX1276 RESET
#define LORA_IO0  (23) // LoRa IRQ
#define LORA_IO1  (23) // Pin tied via diode to DIO0
#define LORA_IO2  (23) // Pin tied via diode to DIO0

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH  (16)      // pin for switching wifi antenna
#define WIFI_ANTENNA 0              // 0 = internal, 1 = external

// uncomment this only if your LoPy runs on a PYTRACK BOARD
//#define HAS_GPS 1
//#define GPS_I2C GPIO_NUM_25, GPIO_NUM_26 // SDA (P22), SCL (P21)
//#define GPS_ADDR 0x10

// uncomment this only if your LoPy runs on a EXPANSION BOARD
//#define HAS_LED (12) // use if LoPy is on Expansion Board, this has a user LED
//#define LED_ACTIVE_LOW 1 // use if LoPy is on Expansion Board, this has a user LED
//#define HAS_BUTTON (13) // user button on expansion board
//#define BUTTON_PULLUP 1  // Button need pullup instead of default pulldown
//#define HAS_BATTERY_PROBE ADC1_GPIO39_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
//#define BATT_FACTOR 2 // voltage divider 1MOhm/1MOhm -> expansion board 3.0
//#define BATT_FACTOR 4 // voltage divider 115kOhm/56kOhm -> expansion board 2.0

#endif
