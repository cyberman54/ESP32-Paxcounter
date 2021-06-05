// clang-format off
// upload_speed 115200
// board esp32dev
// display_library lib_deps_oled_display

#ifndef _ESPGATEWAY_H
#define _ESPGATEWAY_H

#include <stdint.h>

// Hardware related definitions for the ThingPulse ESPGateway.
// The ESPGateway is a device with two ESP32 WROVER boards and
// each one of them can be programmed separately
// Read more about the device here:
// https://thingpulse.com/new-product-the-espgateway-design/
// https://thingpulse.com/the-espgateway-applications/
//
// The other ESP32 module should be flashed with
// https://github.com/squix78/ESP32-Paxcounter-ESPGateway


//#define CFG_sx1276_radio 0 // select LoRa chip
#define BOARD_HAS_PSRAM // use if board has external PSRAM
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED NOT_A_PIN // on board  LED
#define RGB_LED_COUNT 2
#define HAS_RGB_LED SmartLed rgb_led(LED_WS2812B, RGB_LED_COUNT, GPIO_NUM_32, 0) // WS2812B RGB LED on GPIO32

#define HAS_SERIAL 1
#define SERIAL_RXD 14
#define SERIAL_TXD 15
#define BAUDRATE 9600

#endif // end ESPGATEWAY_H
