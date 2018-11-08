#ifndef _EBOXTUBE_H
#define _EBOXTUBE_H

#include <stdint.h>

// Hardware related definitions for ebox ESP32-bit with external connected RFM95 LoRa

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

#define HAS_LED (22)     // Green LED on board
#define HAS_RGB_LED (2)  // WS2812B RGB LED on board
#define HAS_BUTTON (0)   // button "FLASH" on board
#define DISABLE_BROWNOUT 1      // comment out if you want to keep brownout feature

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  (14)
#define LORA_IO0  (26)
#define LORA_IO1  (33)
#define LORA_IO2  (32)

#endif