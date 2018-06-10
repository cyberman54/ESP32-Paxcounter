// Basic Config
#include "globals.h"

#ifdef HAS_RGB_LED

// RGB Led instance
SmartLed rgb_led(LED_WS2812, 1, HAS_RGB_LED);

float rgb_CalcColor(float p, float q, float t)
{
    if (t < 0.0f)
        t += 1.0f;
    if (t > 1.0f)
        t -= 1.0f;

    if (t < 1.0f / 6.0f)
        return p + (q - p) * 6.0f * t;

    if (t < 0.5f)
        return q;

    if (t < 2.0f / 3.0f)
        return p + ((q - p) * (2.0f / 3.0f - t) * 6.0f);

    return p;
}

// ------------------------------------------------------------------------
// Hue, Saturation, Lightness color members
// HslColor using H, S, L values (0.0 - 1.0)
// L should be limited to between (0.0 - 0.5)
// ------------------------------------------------------------------------
RGBColor rgb_hsl2rgb(float h, float s, float l)
{
    RGBColor RGB_color;
    float r;
    float g;
    float b;

    if (s == 0.0f || l == 0.0f)
    {
        r = g = b = l; // achromatic or black
    }
    else
    {
        float q = l < 0.5f ? l * (1.0f + s) : l + s - (l * s);
        float p = 2.0f * l - q;
        r = rgb_CalcColor(p, q, h + 1.0f / 3.0f);
        g = rgb_CalcColor(p, q, h);
        b = rgb_CalcColor(p, q, h - 1.0f / 3.0f);
    }

    RGB_color.R = (uint8_t)(r * 255.0f);
    RGB_color.G = (uint8_t)(g * 255.0f);
    RGB_color.B = (uint8_t)(b * 255.0f);

    return RGB_color;
}

void rgb_set_color(uint16_t hue) {
  if (hue == COLOR_NONE) {
    // Off
    rgb_led[0] = Rgb(0,0,0);
  } else {
    // see http://www.workwithcolor.com/blue-color-hue-range-01.htm
    // H (is color from 0..360) should be between 0.0 and 1.0
    // S is saturation keep it to 1
    // L is brightness should be between 0.0 and 0.5
    // cfg.rgblum is between 0 and 100 (percent)
    RGBColor target = rgb_hsl2rgb( hue / 360.0f, 1.0f, 0.005f * cfg.rgblum);
    //uint32_t color = target.R<<16 | target.G<<8 | target.B;
    rgb_led[0] = Rgb(target.R, target.G, target.B);
  }
  // Show
  rgb_led.show();
}

#else

// No RGB LED empty functions
void rgb_set_color(uint16_t hue) {}

#endif
