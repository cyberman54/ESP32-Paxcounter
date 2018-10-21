#ifndef _OCTOPUS_H
#define _OCTOPUS_H

#include <stdint.h>

// Hardware related definitions for #IoT Octopus32 with the Adafruit LoRaWAN Wing
// You can use this configuration also with the Adafruit ESP32 Feather + the LoRaWAN Wing
// In this config we use the Adafruit OLED Wing which is only 128x32 pixel, need to find a smaller font

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED        13  // ESP32 GPIO12 (pin22) On Board LED
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
//#define HAS_RGB_LED   13  // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED
//#define HAS_BUTTON    15  // ESP32 GPIO15 (pin15) Button is on the LoraNode32 shield
//#define BUTTON_PULLUP  1  // Button need pullup instead of default pulldown

#define HAS_LORA  1       // comment out if device shall not send data via LoRa
#define HAS_SPI   1       // comment out if device shall not send data via SPI
#define CFG_sx1276_radio 1 // RFM95 module

/* SPI remapping does currently not work!! */
// re-define pin definitions of pins_arduino.h
#define PIN_SPI_SS    14 //14 // ESP32  GPIO5 (Pin5)  -- SX1276 NSS  (Pin19) SPI Chip Select Input
#define PIN_SPI_MOSI  18 // ESP32 GPIO23 (Pin23) -- SX1276 MOSI (Pin18) SPI Data Input
#define PIN_SPI_MISO  19 // ESP32 GPIO19 (Pin19) -- SX1276 MISO (Pin17) SPI Data Output
#define PIN_SPI_SCK   5 // ESP32 GPIO18 (Pin18) -- SX1276 SCK  (Pin16) SPI Clock Input

// Pins for LORA chip reset and interrupt lines
#define RST   LMIC_UNUSED_PIN
#define DIO0  (33)
#define DIO1  (33)
#define DIO2  LMIC_UNUSED_PIN
#define DIO5  LMIC_UNUSED_PIN

// Pins for I2C interface of OLED Display
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // U8X8_SSD1306_128X32_UNIVISION_SW_I2C //
//#define DISPLAY_FLIP  1 // uncomment this for rotated display 
#define MY_OLED_SDA (23)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE 

#endif
