// Basic Config
#include "globals.h"
#include "led.h"

led_states LEDState = LED_OFF; // LED state global for state machine
led_states previousLEDState =
    LED_ON; // This will force LED to be off at boot since State is OFF

TaskHandle_t ledLoopTask;

uint16_t LEDColor = COLOR_NONE, LEDBlinkDuration = 0; // state machine variables
unsigned long LEDBlinkStarted = 0; // When (in millis() led blink started)

#ifdef HAS_RGB_LED

// RGB Led instance
HAS_RGB_LED;

float rgb_CalcColor(float p, float q, float t) {
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
RGBColor rgb_hsl2rgb(float h, float s, float l) {
  RGBColor RGB_color;
  float r;
  float g;
  float b;

  if (s == 0.0f || l == 0.0f) {
    r = g = b = l; // achromatic or black
  } else {
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
    rgb_led[0] = Rgb(0, 0, 0);
  } else {
    // see http://www.workwithcolor.com/blue-color-hue-range-01.htm
    // H (is color from 0..360) should be between 0.0 and 1.0
    // S is saturation keep it to 1
    // L is brightness should be between 0.0 and 0.5
    // cfg.rgblum is between 0 and 100 (percent)
    RGBColor target = rgb_hsl2rgb(hue / 360.0f, 1.0f, 0.005f * cfg.rgblum);
    // uint32_t color = target.R<<16 | target.G<<8 | target.B;
    rgb_led[0] = Rgb(target.R, target.G, target.B);
  }
  // Show
  rgb_led.show();
}

#else

// No RGB LED empty functions
void rgb_set_color(uint16_t hue) {}

#endif

void switch_LED(uint8_t state) {
#if (HAS_LED != NOT_A_PIN)
  if (state == LED_ON) {
    // switch LED on
#ifdef LED_ACTIVE_LOW
    digitalWrite(HAS_LED, LOW);
#else
    digitalWrite(HAS_LED, HIGH);
#endif
  } else if (state == LED_OFF) {
    // switch LED off
#ifdef LED_ACTIVE_LOW
    digitalWrite(HAS_LED, HIGH);
#else
    digitalWrite(HAS_LED, LOW);
#endif
  }
#endif
}

void switch_LED1(uint8_t state) {
#ifdef HAS_TWO_LED
  if (state == LED_ON) {
    // switch LED on
#ifdef LED1_ACTIVE_LOW
    digitalWrite(HAS_TWO_LED, LOW);
#else
    digitalWrite(HAS_TWO_LED, HIGH);
#endif
  } else if (state == LED_OFF) {
    // switch LED off
#ifdef LED1_ACTIVE_LOW
    digitalWrite(HAS_TWO_LED, HIGH);
#else
    digitalWrite(HAS_TWO_LED, LOW);
#endif
  }
#endif // HAS_TWO_LED
}

void blink_LED(uint16_t set_color, uint16_t set_blinkduration) {
#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  LEDColor = set_color;                 // set color for RGB LED
  LEDBlinkDuration = set_blinkduration; // duration
  LEDBlinkStarted = millis();           // Time Start here
  LEDState = LED_ON;                    // Let main set LED on
#endif
}

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)

void ledLoop(void *parameter) {
  while (1) {
    // Custom blink running always have priority other LoRaWAN led
    // management
    if (LEDBlinkStarted && LEDBlinkDuration) {
      // Custom blink is finished, let this order, avoid millis() overflow
      if ((millis() - LEDBlinkStarted) >= LEDBlinkDuration) {
        // Led becomes off, and stop blink
        LEDState = LED_OFF;
        LEDBlinkStarted = 0;
        LEDBlinkDuration = 0;
        LEDColor = COLOR_NONE;
      } else {
        // In case of LoRaWAN led management blinked off
        LEDState = LED_ON;
      }
      // No custom blink, check LoRaWAN state
    } else {

#if (HAS_LORA)
      // LED indicators for viusalizing LoRaWAN state
      if (LMIC.opmode & (OP_JOINING | OP_REJOIN)) {
        LEDColor = COLOR_YELLOW;
        // quick blink 20ms on each 1/5 second
        LEDState =
            ((millis() % 200) < 20) ? LED_ON : LED_OFF; // TX data pending
      } else if (LMIC.opmode & (OP_TXDATA | OP_TXRXPEND)) {
        // select color to blink by message port
        switch (LMIC.pendTxPort) {
        case STATUSPORT:
          LEDColor = COLOR_RED;
          break;
        case CONFIGPORT:
          LEDColor = COLOR_CYAN;
          break;
        default:
          LEDColor = COLOR_BLUE;
          break;
        }
        // small blink 10ms on each 1/2sec (not when joining)
        LEDState = ((millis() % 500) < 10) ? LED_ON : LED_OFF;
        // This should not happen so indicate a problem
      } else if (LMIC.opmode &
                 ((OP_TXDATA | OP_TXRXPEND | OP_JOINING | OP_REJOIN) == 0)) {
        LEDColor = COLOR_RED;
        // heartbeat long blink 200ms on each 2 seconds
        LEDState = ((millis() % 2000) < 200) ? LED_ON : LED_OFF;
      } else
#endif // HAS_LORA
      {
        // led off
        LEDColor = COLOR_NONE;
        LEDState = LED_OFF;
      }
    }
    // led need to change state? avoid digitalWrite() for nothing
    if (LEDState != previousLEDState) {
      if (LEDState == LED_ON) {
        rgb_set_color(LEDColor);
        // if we have only single LED we use it to blink for status
#ifndef HAS_RGB_LED
        switch_LED(LED_ON);
#endif
      } else {
        rgb_set_color(COLOR_NONE);
#ifndef HAS_RGB_LED
        switch_LED(LED_OFF);
#endif
      }
      previousLEDState = LEDState;
    }
    // give yield to CPU
    delay(2);
  }                         // while(1)
};                          // ledloop()

#endif // #if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
