#ifndef _LED_H
#define _LED_H

#ifdef HAS_RGB_LED
#define FASTLED_INTERNAL
#include <FastLED.h>
#include "libpax_helpers.h"
#endif

#ifdef HAS_LORA
#include "lorawan.h"
#endif

#ifndef RGB_LED_COUNT
#define RGB_LED_COUNT 1
#endif

enum led_states { LED_OFF, LED_ON };

enum colors {
  COLOR_WHITE = 0xFFFFFF,
  COLOR_NONE = 0x000000,
  COLOR_CYAN = 0x00FFFF,
  COLOR_BLUE = 0x0000FF,
  COLOR_GREEN = 0x008000,
  COLOR_YELLOW = 0xFFFF00,
  COLOR_ORANGE = 0xFFA500,
  COLOR_RED = 0xFF0000,
  COLOR_PINK = 0xFFC0CB
};

extern TaskHandle_t ledLoopTask;

void rgb_led_init(void);
void rgb_set_color(uint32_t color);
void ledLoop(void *parameter);
void switch_LED(led_states state);
void switch_LED1(led_states state);

#endif