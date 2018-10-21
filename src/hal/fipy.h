#ifndef _FIPY_H
#define _FIPY_H

#include <stdint.h>

// Hardware related definitions for Pycom FiPy Board

#define HAS_LORA 1 // comment out if device shall not send data via LoRa
#define HAS_SPI 1  // comment out if device shall not send data via SPI

#define CFG_sx1272_radio 1
#define HAS_LED NOT_A_PIN      // FiPy has no on board LED, so we use RGB LED
#define HAS_RGB_LED GPIO_NUM_0 // WS2812B RGB LED on GPIO0
#define BOARD_HAS_PSRAM        // use extra 4MB extern RAM

// Pins for LORA chip reset and interrupt lines
#define RST   LMIC_UNUSED_PIN
#define DIO0  (23) // LoRa IRQ
#define DIO1  (23) // Pin tied via diode to DIO0
#define DIO2  LMIC_UNUSED_PIN

// select WIFI antenna (internal = onboard / external = u.fl socket)
#define HAS_ANTENNA_SWITCH GPIO_NUM_21 // pin for switching wifi antenna
#define WIFI_ANTENNA 0                 // 0 = internal, 1 = external

#endif
