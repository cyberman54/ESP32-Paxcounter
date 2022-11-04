// clang-format off
// upload_speed 1500000
// board ESP32-S3-DevKitC-1

// see https://github.com/Xinyuan-LilyGO/T-Dongle-S3

#ifndef _TTGOTDONGLES3_H
#define _TTGOTDONGLES3_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN
#define HAS_BUTTON 0

#define HAS_SDCARD  2      // this board has a SD MMC card-reader/writer
#define SDCARD_SLOTWIDTH 4 // 4-line interface

#define HAS_DISPLAY 2       // TFT-LCD
#define TFT_TYPE DISPLAY_TDONGLE_S3
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