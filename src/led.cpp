// Basic Config
#include "globals.h"
#include "led.h"

static led_states LEDState = LED_OFF; // LED state global for state machine
TaskHandle_t ledLoopTask;
static uint16_t LEDBlinkDuration = 0; // state machine variables
static unsigned long LEDBlinkStarted =
    0; // When (in millis() led blink started)

#ifdef HAS_RGB_LED
CRGB leds[RGB_LED_COUNT];
#endif

void led_setcolor(CRGB color) {
#ifdef HAS_RGB_LED
  for (int i = 0; i < RGB_LED_COUNT; i++)
    leds[i] = color;
  FastLED.show();
#endif
}

void led_sethue(uint8_t hue) {
#ifdef HAS_RGB_LED
  for (int i = 0; i < RGB_LED_COUNT; i++)
    leds[i] = CHSV(hue, 0XFF, 100);
  FastLED.show();
#endif
}

void rgb_led_init(void) {
#ifdef HAS_RGB_LED
  HAS_RGB_LED;
  led_setcolor(CRGB::Green);
#endif
}

void switch_LED(led_states state) {
  static led_states previousLEDState = LED_OFF;
  // led need to change state? avoid digitalWrite() for nothing
  if (state != previousLEDState) {
    previousLEDState = state;
#if (HAS_LED != NOT_A_PIN)
    if (state == LED_ON) {
      // switch LED on
#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_LED, LOW);
#else
      digitalWrite(HAS_LED, HIGH);
#endif
      led_setcolor(CRGB::White);
    } else if (state == LED_OFF) {
      // switch LED off
#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_LED, HIGH);
#else
      digitalWrite(HAS_LED, LOW);
#endif
      led_setcolor(CRGB::Black);
    }
#endif
  }
}

void switch_LED1(led_states state) {
  static led_states previousLEDState = LED_OFF;
  // led need to change state? avoid digitalWrite() for nothing
  if (state != previousLEDState) {
    previousLEDState = state;
#if (HAS_LED != NOT_A_PIN)
    if (state == LED_ON) {
      // switch LED on
#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_TWO_LED, LOW);
#else
      digitalWrite(HAS_TWO_LED, HIGH);
#endif
    } else if (state == LED_OFF) {
      // switch LED off
#ifdef LED_ACTIVE_LOW
      digitalWrite(HAS_TWO_LED, HIGH);
#else
      digitalWrite(HAS_TWO_LED, LOW);
#endif
    }
#endif
  }
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
        led_setcolor(CRGB::Black);
      } else {
        // In case of LoRaWAN led management blinked off
        LEDState = LED_ON;
      }
      // No custom blink, check LoRaWAN state
    } else {
#if (HAS_LORA)
      // LED indicators for viusalizing LoRaWAN state
      if (LMIC.opmode & (OP_JOINING | OP_REJOIN)) {
        led_setcolor(CRGB::Yellow);
        // quick blink 20ms on each 1/5 second
        LEDState =
            ((millis() % 200) < 20) ? LED_ON : LED_OFF; // TX data pending
      } else if (LMIC.opmode & (OP_TXDATA | OP_TXRXPEND)) {
        // select color to blink by message port
        switch (LMIC.pendTxPort) {
        case STATUSPORT:
          led_setcolor(CRGB::Pink);
          break;
        case CONFIGPORT:
          led_setcolor(CRGB::Cyan);
          break;
        default:
          led_setcolor(CRGB::Blue);
          break;
        }
        // small blink 10ms on each 1/2sec (not when joining)
        LEDState = ((millis() % 500) < 10) ? LED_ON : LED_OFF;
        // This should not happen so indicate a problem
      } else if (LMIC.opmode &
                 ((OP_TXDATA | OP_TXRXPEND | OP_JOINING | OP_REJOIN) == 0)) {
        led_setcolor(CRGB::Red);
        // heartbeat long blink 200ms on each 2 seconds
        LEDState = ((millis() % 2000) < 200) ? LED_ON : LED_OFF;
      } else
#elif (defined(HAS_RGB_LED) && ((WIFICOUNTER) || (BLECOUNTER)))
      struct count_payload_t count;
      libpax_counter_count(&count);
      led_sethue(count.pax);
#endif // HAS_LORA
      {
        // led off
        LEDState = LED_OFF;
      }
    }

    switch_LED(LEDState);

    // give yield to CPU
    delay(5);
  } // while(1)
};  // ledloop()

#endif // #if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
