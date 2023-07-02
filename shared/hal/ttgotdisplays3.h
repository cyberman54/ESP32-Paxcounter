// clang-format off
// upload_speed 1500000
// board esp32-s3-devkitc-1

#ifndef _TTGOTDISPLAYS3_H
#define _TTGOTDISPLAYS3_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN
#define HAS_BUTTON 14       // on board button (right side)

// power management settings
//#define BAT_MEASURE_ADC ADC1_GPIO4_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_6
//#define BAT_VOLTAGE_DIVIDER 2.605f         // voltage divider

// Display Settings
#define HAS_DISPLAY 2       // TFT-LCD parallel
#define TFT_TYPE DISPLAY_T_DISPLAY_S3
#define MY_DISPLAY_FLIP  1  // use if display is rotated
#define MY_DISPLAY_WIDTH 172
#define MY_DISPLAY_HEIGHT 320

#endif