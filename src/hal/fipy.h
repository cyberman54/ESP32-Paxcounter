// clang-format off
// upload_speed 921600
// board esp32dev

#ifndef _FIPY_H
#define _FIPY_H

#include <stdint.h>

// Hardware related definitions for Pycom FiPy Board

#define HAS_LORA 1 // comment out if device shall not send data via LoRa

#define CFG_sx1272_radio 1
#define HAS_LED NOT_A_PIN      // FiPy has no on board LED, so we use RGB LED
#define HAS_RGB_LED SmartLed rgb_led(LED_WS2812, 1, GPIO_NUM_0) // WS2812B RGB LED on GPIO0
#define BOARD_HAS_PSRAM        // use extra 4MB extern RAM

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  LMIC_UNUSED_PIN
#define LORA_IRQ  (23) // LoRa IRQ
#define LORA_IO1  (23) // Pin tied via diode to DIO0
#define LORA_IO2  LMIC_UNUSED_PIN

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH GPIO_NUM_21 // pin for switching wifi antenna
#define WIFI_ANTENNA 0                 // 0 = internal, 1 = external

#endif
