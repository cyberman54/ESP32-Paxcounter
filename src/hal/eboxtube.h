#ifndef _EBOXTUBE_H
#define _EBOXTUBE_H

#include <stdint.h>

// Hardware related definitions for ebox ESP32-bit with external connected RFM95 LoRa

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1

#define HAS_LED (22)     // Green LED on board
#define HAS_RGB_LED (2)  // WS2812B RGB LED on board
#define HAS_BUTTON (0)   // button "FLASH" on board
#define DISABLE_BROWNOUT 1      // comment out if you want to keep brownout feature

// Pins for LORA chip reset and interrupt lines
#define RST   (14)
#define DIO0  (26)
#define DIO1  (33)
#define DIO2  (32)

#endif