// clang-format off
// upload_speed 921600
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
#define TFT_TYPE DISPLAY_T_DISPLAY
#define MY_DISPLAY_FLIP  1  // use if display is rotated
#define MY_DISPLAY_WIDTH 135
#define MY_DISPLAY_HEIGHT 240

#endif
