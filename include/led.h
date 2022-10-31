#ifndef _LED_H
#define _LED_H

#ifdef HAS_RGB_LED
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
extern TaskHandle_t ledLoopTask;

void rgb_led_init(void);
void led_sethue(uint8_t hue);
void ledLoop(void *parameter);
void switch_LED(led_states state);
void switch_LED1(led_states state);

#endif