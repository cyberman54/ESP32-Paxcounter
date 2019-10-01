// clang-format off
// upload_speed 921600
// board featheresp32

#ifndef _OCTOPUS_H
#define _OCTOPUS_H
#include <stdint.h>

// Hardware related definitions for #IoT Octopus32 with the Adafruit LoRaWAN Wing
// You can use this configuration also with the Adafruit ESP32 Feather + the LoRaWAN Wing
// In this config we use the Adafruit OLED Wing which is only 128x32 pixel, need to find a smaller font

// disable brownout detection (avoid unexpected reset on some boards)
#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

// Octopus32 has a pre-populated BME680 on i2c addr 0x76
#define HAS_BME 1 // Enable BME sensors in general
#define HAS_BME680 GPIO_NUM_23, GPIO_NUM_22 // SDA, SCL
#define BME680_ADDR BME680_I2C_ADDR_PRIMARY // connect SDIO of BME680 to GND

// user defined sensors
//#define HAS_SENSORS 1 // comment out if device has user defined sensors

#define HAS_LED        13  // ESP32 GPIO12 (pin22) On Board LED
//#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
//#define HAS_RGB_LED SmartLed rgb_led(LED_WS2812, 1, GPIO_NUM_13) // ESP32 GPIO13 (pin13) On Board Shield WS2812B RGB LED
//#define HAS_BUTTON    15  // ESP32 GPIO15 (pin15) Button is on the LoraNode32 shield
//#define BUTTON_PULLUP  1  // Button need pullup instead of default pulldown

#define HAS_LORA  1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1 // RFM95 module

// Pins for LORA chip SPI interface, reset line and interrupt lines
#define LORA_SCK  (5) 
#define LORA_CS   (14)
#define LORA_MISO (19)
#define LORA_MOSI (18)
#define LORA_RST  LMIC_UNUSED_PIN
#define LORA_IRQ  (33)
#define LORA_IO1  (33)
#define LORA_IO2  LMIC_UNUSED_PIN

// Pins for I2C interface of OLED Display
#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // U8X8_SSD1306_128X32_UNIVISION_SW_I2C //
//#define DISPLAY_FLIP  1 // uncomment this for rotated display 
#define MY_OLED_SDA (23)
#define MY_OLED_SCL (22)
#define MY_OLED_RST U8X8_PIN_NONE 

#endif
