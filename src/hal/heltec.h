#ifndef _HELTEC_H
#define _HELTEC_H

#include <stdint.h>

// Hardware related definitions for Heltec LoRa-32 Board
#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED (25) // white LED on board
#define HAS_BUTTON (0) // button "PROG" on board

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (4)
#define MY_OLED_SCL (15)
#define MY_OLED_RST (16)

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  (14)
#define LORA_IO0  (26)
#define LORA_IO1  (33)
#define LORA_IO2  LMIC_UNUSED_PIN

#endif
