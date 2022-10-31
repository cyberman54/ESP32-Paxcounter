// Basic Config
#include "globals.h"
#include "led.h"

static led_states LEDState = LED_OFF; // LED state global for state machine
led_states previousLEDState =
    LED_ON; // This will force LED to be off at boot since State is OFF

TaskHandle_t ledLoopTask;

uint32_t LEDColor = COLOR_NONE, LEDBlinkDuration = 0; // state machine variables
static unsigned long LEDBlinkStarted =
    0; // When (in millis() led blink started)

#ifdef HAS_RGB_LED
CRGB leds[RGB_LED_COUNT];
#endif

void rgb_set_color(uint32_t color) {
#ifdef HAS_RGB_LED
  for (int i = 0; i < RGB_LED_COUNT; i++)
    leds[i] = CRGB(color);
  FastLED.show();
#endif
}

void rgb_led_init(void) {
#ifdef HAS_RGB_LED
  HAS_RGB_LED;
  rgb_set_color(COLOR_NONE);
#endif
}

void switch_LED(led_states state) {
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

void switch_LED1(led_states state) {
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


#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)

void ledLoop(void *parameter) {
  while (1) {
    // Custom blink running always have priority other LoRaWAN led
    // management
    if (LEDBlinkStarted && LEDBlinkDuration) {
      // Custom blink is finished, let this order, avoid millis() overflow
      if ((long)(millis() - LEDBlinkStarted) >= LEDBlinkDuration) {
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
          LEDColor = COLOR_PINK;
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
    delay(5);
  } // while(1)
};  // ledloop()

#endif // #if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)