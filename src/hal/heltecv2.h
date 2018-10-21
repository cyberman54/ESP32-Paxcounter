#ifndef _HELTECV2_H
#define _HELTECV2_H

#include <stdint.h>

// Hardware related definitions for Heltec V2 LoRa-32 Board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define HAS_SPI 1        // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
#define HAS_LED (25) // white LED on board
#define HAS_BUTTON (0) // button "PROG" on board

// Pins for I2C interface of OLED Display
#define OLED_SDA (4)
#define OLED_SCL (15)
#define OLED_RST (16)

// Pins for LORA chip reset and interrupt lines
#define RST   (14)
#define DIO0  (26)
#define DIO1  (34)
#define DIO2  (35)

#endif