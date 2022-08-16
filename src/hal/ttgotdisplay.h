// clang-format off
// upload_speed 1500000
// board esp32dev

#ifndef _TTGOTDISPLAY_H
#define _TTGOTDISPLAY_H

#include <stdint.h>

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_LED NOT_A_PIN  // no on board LED (?)
#define HAS_BUTTON (35)    // on board button A

// power management settings
#define BAT_MEASURE_ADC ADC1_GPIO34_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_6
#define BAT_VOLTAGE_DIVIDER 2.605f           // voltage divider

// Display Settings
#define HAS_DISPLAY 2       // TFT-LCD
#define MY_DISPLAY_FLIP  1  // use if display is rotated
#define MY_DISPLAY_WIDTH 135
#define MY_DISPLAY_HEIGHT 240
#define MY_DISPLAY_INVERT 1
#define TFT_TYPE LCD_ST7789_135 // size 135x240 px
#define TFT_MOSI GPIO_NUM_19 // SPI
#define TFT_MISO NOT_A_PIN   // SPI
#define TFT_SCLK GPIO_NUM_18 // SPI
#define TFT_CS   GPIO_NUM_5  // Chip select control
#define TFT_DC   GPIO_NUM_16 // Data Command control
#define TFT_RST  GPIO_NUM_23 // Reset
#define TFT_BL   GPIO_NUM_4  // LED back-light
#define TFT_FREQUENCY 40000000

#endif
