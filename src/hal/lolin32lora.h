// clang-format off
// upload_speed 921600
// board lolin32

#ifndef _LOLINLORA_H
#define _LOLINLORA_H

#include <stdint.h>

// Hardware related definitions for lolin32 with loraNode32 shield
// See https://github.com/hallard/LoLin32-Lora

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED NOT_A_PIN // Led os on same pin as Lora SS pin, to avoid problems, we don't use it
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
                          // Anyway shield is on over the LoLin32 board, so we won't be able to see this LED
#define HAS_RGB_LED SmartLed rgb_led(LED_WS2812, 1, GPIO_NUM_13) // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED
#define HAS_BUTTON    15  // ESP32 GPIO15 (pin15) Button is on the LoraNode32 shield
#define BUTTON_PULLUP  1  // Button need pullup instead of default pulldown

#define HAS_LORA  1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // RFM95 module

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (18)
#define LORA_MISO (19)
#define LORA_MOSI (27)
#define LORA_RST  (25)
#define LORA_IRQ  (27)
#define LORA_IO1  (26)
#define LORA_IO2  LMIC_UNUSED_PIN
#define LORA_IO5  LMIC_UNUSED_PIN

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (21)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE

// I2C config for Microchip 24AA02E64 DEVEUI unique address
#define MCP_24AA02E64_I2C_ADDRESS 0x50 // I2C address for the 24AA02E64 
#define MCP_24AA02E64_MAC_ADDRESS 0xF8 // Memory adress of unique deveui 64 bits

#endif
