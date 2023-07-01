// clang-format off
// upload_speed 921600
// board lolin32

#ifndef _WEMOS32MATRIX_H
#define _WEMOS32MATRIX_H

#include <stdint.h>

#define HAS_LED NOT_A_PIN // no LED

// LED Matrix display settings
#define HAS_MATRIX_DISPLAY              1       // Uncomment to enable LED matrix display output
#define LED_MATRIX_WIDTH                (32*2)  // Width (cols) in pixels (LEDs) of your display, must be 32X
#define LED_MATRIX_HEIGHT               (16*1)  // Height (rows) in pixels (LEDs) of your display, must be 16X

// Pin numbers work fine for Wemos Lolin32 board (all used pins are on 1 side of the board)
// Explanation of pin signals see https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/new-wiring
#define MATRIX_DISPLAY_SCAN_US          500     // Matrix display scan rate in microseconds (1ms is about 'acceptable')
#define LED_MATRIX_LATCHPIN             13      // LAT (or STB = Strobe)
#define LED_MATRIX_CLOCKPIN             32      // CLK
#define LED_MATRIX_EN_74138             12      // EN (or OE)
#define LED_MATRIX_LA_74138             14      // LA (or A)
#define LED_MATRIX_LB_74138             27      // LB (or B)
#define LED_MATRIX_LC_74138             25      // LC (or C)
#define LED_MATRIX_LD_74138             26      // LD (or D)
#define LED_MATRIX_DATA_R1              33      // R1 (or R0)

// CLK: The clock signal moves the data bits from pin R1 ("red") in the shift registers
// LAT: The latch signal enables LEDs according to the shift register's contents
// Line Selects: LA, LB, LC, LD select which rows of the display are currently lit (0 .. 15)
// OE: Output enable switches the LEDs on/off while transitioning from one row to the next

/*
How it works:

1. clock out 8 bytes for columns via R1 and CLK (8 * 8 bit -> 64 columns)
2. OE disable (LEDs off)
3. select line to lit with LA/LB/LC/LD hex coded (4 bit -> 16 rows)
4. latch data from shift registers to columns
5. OE enable (LEDs on)
6. repeat with step1 for next line
*/

#define DISABLE_BROWNOUT 1 // comment out if you want to keep brownout feature

#endif