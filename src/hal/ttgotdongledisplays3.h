// clang-format off
// upload_speed 1500000
// board esp32-s3-devkitc-1

// for pinouts see https://github.com/Xinyuan-LilyGO/T-Dongle-S3

#ifndef _TTGOTDONGLEDISPLAYS3_H
#define _TTGOTDONGLEDISPLAYS3_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN
#define RGB_LED_COUNT 1
#define HAS_RGB_LED FastLED.addLeds<APA102, 40, 39, BGR>(leds, RGB_LED_COUNT)
#define FASTLED_INTERNAL
#define HAS_BUTTON 0       // dongle button is on GPIO0
#define HAS_SDCARD  2      // dongle has a SD MMC card-reader/writer
#define SDCARD_SLOTWIDTH 4 // dongle has 4 line interface
#define SDCARD_SLOTCONFIG { .clk = GPIO_NUM_12, .cmd = GPIO_NUM_16, .d0 = GPIO_NUM_14, .d1 = GPIO_NUM_17, .d2 = GPIO_NUM_21, .d3 = GPIO_NUM_18, .cd = SDMMC_SLOT_NO_CD, .wp = SDMMC_SLOT_NO_WP, .width = 4, .flags = 0, }

#define HAS_DISPLAY 2       // TFT-LCD
#define TFT_TYPE DISPLAY_T_DISPLAY_S3 // may currently not work, we are awaiting a PR for display lib
#define MY_DISPLAY_FLIP  1  // use if display is rotated
#define MY_DISPLAY_WIDTH 80
#define MY_DISPLAY_HEIGHT 160

#define TFT_CS_PIN     4
#define TFT_SDA_PIN    3
#define TFT_SCL_PIN    5
#define TFT_DC_PIN     2
#define TFT_RES_PIN    1
#define TFT_LEDA_PIN   38

#endif