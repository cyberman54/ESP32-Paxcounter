// clang-format off
// upload_speed 1500000
// board pico32

#ifndef _TTGOTWRISTBAND_H
#define _TTGOTWRISTBAND_H

#include <stdint.h>

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#define HAS_DISPLAY 2      // TFT-LCD, support work in progess, not ready yet
#define MY_DISPLAY_FLIP  1 // use if display is rotated

#define HAS_LED NOT_A_PIN  // no on board LED (?)
#define HAS_BUTTON (25)    // on board button A

// power management settings
#define BAT_MEASURE_ADC ADC1_GPIO34_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_6
#define BAT_VOLTAGE_DIVIDER 2.605f           // voltage divider

// Display Settings
#define MY_DISPLAY_WIDTH 80
#define MY_DISPLAY_HEIGHT 160
#define MY_DISPLAY_INVERT 1

// setting for TTGO T-display
#define USER_SETUP_LOADED 1
#define ST7735_DRIVER 1

#define CGRAM_OFFSET

#define TFT_MISO -1
#define TFT_MOSI GPIO_NUM_19 // SPI
#define TFT_SCLK GPIO_NUM_18 // SPI
#define TFT_CS   GPIO_NUM_5  // Chip select control
#define TFT_DC   GPIO_NUM_23 // Data Command control
#define TFT_RST  GPIO_NUM_26 // Reset
#define TFT_BL   GPIO_NUM_27 // LED back-light
#define TFT_BACKLIGHT_ON 1
#define ST7735_GREENTAB160x80

#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY 6000000

#endif

