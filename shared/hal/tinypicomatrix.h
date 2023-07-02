// clang-format off
// upload_speed 921600
// board esp32dev


#ifndef _TINYPICO_H
#define _TINYPICO_H

#include <stdint.h>

// Hardware related definitions for crowdsupply tinypico board
// for operating a 96x16 shift register LED matrix display

#define HAS_LED NOT_A_PIN     // Green LED on board
#define HAS_RGB_LED FastLED.addLeds<APA102, GPIO_NUM_2, GPIO_NUM_12, BGR>(leds, RGB_LED_COUNT)

//#define DISABLE_BROWNOUT 1      // comment out if you want to keep brownout feature
#define BAT_MEASURE_ADC ADC1_GPIO35_CHANNEL // battery probe GPIO pin -> ADC1_CHANNEL_7
#define BAT_VOLTAGE_DIVIDER 2.7625f // voltage divider 160k/442k on board
#define BOARD_HAS_PSRAM // use extra 4MB external RAM
#define LED_POWER_SW (13) // switches LED power
#define LED_POWER_ON 0 // switch on transistor for LED power
#define LED_POWER_OFF 1

// LED Matrix display settings
#define HAS_MATRIX_DISPLAY              1       // Uncomment to enable LED matrix display output
#define LED_MATRIX_WIDTH                (32*2)  // Width (cols) in pixels (LEDs) of your display, must be 32X
#define LED_MATRIX_HEIGHT               (16*1)  // Height (rows) in pixels (LEDs) of your display, must be 16X

// Explanation of pin signals see https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/new-wiring
#define MATRIX_DISPLAY_SCAN_US          500     // Matrix display scan rate in microseconds (1ms is about 'acceptable')
#define LED_MATRIX_LATCHPIN             32      // LAT (or STB = Strobe)
#define LED_MATRIX_CLOCKPIN             33      // CLK
#define LED_MATRIX_EN_74138             21      // EN (or OE)
#define LED_MATRIX_LA_74138             23      // LA (or A)
#define LED_MATRIX_LB_74138             19      // LB (or B)
#define LED_MATRIX_LC_74138             18      // LC (or C)
#define LED_MATRIX_LD_74138             5       // LD (or D)
#define LED_MATRIX_DATA_R1              22      // R1 (or R0)

// CLK: The clock signal moves the data bits from pin R1 ("red") in the shift registers
// LAT: The latch signal enables LEDs according to the shift register's contents
// Line Selects: LA, LB, LC, LD select which rows of the display are currently lit (0 .. 15)
// OE: Output enable switches the LEDs on/off while transitioning from one row to the next

#endif