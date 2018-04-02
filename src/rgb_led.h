
#pragma once

// value for HSL color
// see http://www.workwithcolor.com/blue-color-hue-range-01.htm
#define COLOR_RED              0
#define COLOR_ORANGE          30
#define COLOR_ORANGE_YELLOW   45
#define COLOR_YELLOW          60
#define COLOR_YELLOW_GREEN    90
#define COLOR_GREEN          120
#define COLOR_GREEN_CYAN     165
#define COLOR_CYAN           180
#define COLOR_CYAN_BLUE      210
#define COLOR_BLUE           240
#define COLOR_BLUE_MAGENTA   275
#define COLOR_MAGENTA        300
#define COLOR_PINK           350
#define COLOR_WHITE          360
#define COLOR_NONE           999

struct RGBColor
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
};

// Exported Functions
void rgb_set_color(uint16_t hue);
