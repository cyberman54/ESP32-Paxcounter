// clang-format off
// upload_speed 921600
// board lolin32

#ifndef _WEMOS32OLED_H
#define _WEMOS32OLED_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN // no LED

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C
#define MY_OLED_SDA (5)
#define MY_OLED_SCL (4)
#define MY_OLED_RST U8X8_PIN_NONE
#define DISPLAY_FLIP  1 // use if display is rotated

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif
