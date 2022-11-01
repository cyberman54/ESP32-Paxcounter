// clang-format off
// upload_speed 921600
// board esp32dev


#ifndef _TINYPICO_H
#define _TINYPICO_H

#include <stdint.h>

// Hardware related definitions for crowdsupply tinypico board

#define HAS_LED NOT_A_PIN     // Green LED on board
#define HAS_RGB_LED FastLED.addLeds<APA102, GPIO_NUM_2, GPIO_NUM_12, BGR>(leds, RGB_LED_COUNT)

//#define DISABLE_BROWNOUT 1      // comment out if you want to keep brownout feature
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2.7625f // voltage divider 160k/442k on board
#define BOARD_HAS_PSRAM // use extra 4MB external RAM
#define LED_POWER_SW (13) // switches LED power
#define LED_POWER_ON 0 // switch on transistor for LED power
#define LED_POWER_OFF 1

#endif