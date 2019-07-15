// clang-format off
// upload_speed 921600
// board lolin32

#ifndef _WEMOS32LEDMATRIX_H
#define _WEMOS32LEDMATRIX_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN // no LED

// LED Matrix display settings
#define HAS_MATRIX_DISPLAY              1       // Uncomment to enable LED matrix display output
#define LED_MATRIX_WIDTH                64      // Width in pixels (LEDs) of your display
#define LED_MATRIX_HEIGHT               16      // Height in pixels (LEDs ) of your display

// Pin numbers work fine for Wemos Lolin32 board (all used pins are on 1 side of the board)
#define MATRIX_DISPLAY_SCAN_US          500     // Matrix display scan rate in microseconds (1ms is about 'acceptable')
#define LED_MATRIX_LATCHPIN             13      // Connects to LAT pin on display (Latch)
#define LED_MATRIX_CLOCKPIN             32      // Connects to CLK pin on display (Clock)
#define LED_MATRIX_EN_74138             12      // Connects to EN pin on display (Output Enable)
#define LED_MATRIX_LA_74138             14      // Connects to LA pin on display
#define LED_MATRIX_LB_74138             27      // Connects to LB pin on display
#define LED_MATRIX_LC_74138             25      // Connects to LC pin on display
#define LED_MATRIX_LD_74138             26      // Connects to LD pin on display
#define LED_MATRIX_DATA_R1              33      // Connects to R1 pin on display

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif