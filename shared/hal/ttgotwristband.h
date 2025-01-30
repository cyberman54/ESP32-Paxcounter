// clang-format off
// upload_speed 921600
// board pico32

#ifndef _TTGOTWRISTBAND_H
#define _TTGOTWRISTBAND_H

#include <stdint.h>

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED NOT_A_PIN  // no on board LED (?)
#define HAS_BUTTON (33)    // on board button A

// power management settings
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2.605f           // voltage divider

// Display Settings
#define HAS_DISPLAY 2      // TFT-LCD
#define MY_DISPLAY_FLIP  1 // use if display is rotated
#define MY_DISPLAY_WIDTH 80
#define MY_DISPLAY_HEIGHT 160
#define TFT_MOSI GPIO_NUM_19 // SPI
#define TFT_MISO NOT_A_PIN   // SPI
#define TFT_SCLK GPIO_NUM_18 // SPI
#define TFT_CS   GPIO_NUM_5  // Chip select control
#define TFT_DC   GPIO_NUM_23 // Data Command control
#define TFT_RST  GPIO_NUM_26 // Reset
#define TFT_BL   GPIO_NUM_27 // LED back-light
#define TFT_FREQUENCY 27000000
#define TFT_TYPE LCD_ST7735S, FLAGS_NONE, TFT_FREQUENCY, TFT_CS, TFT_DC, TFT_RST, TFT_BL, TFT_MISO, TFT_MOSI, TFT_SCLK

#endif

