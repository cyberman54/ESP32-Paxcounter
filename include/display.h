#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "cyclic.h"
#include "qrcode.h"
#define DISPLAY_PAGES (7) // number of paxcounter display pages

// settings for oled display library
#define USE_BACKBUFFER 1
#define MY_OLED OLED_128x64
#ifdef MY_OLED_ADDR
    #define OLED_ADDR MY_OLED_ADDR
#else
    #define OLED_ADDR -1
#endif
#define OLED_INVERT 0
#define USE_HW_I2C 1

#ifndef DISPLAY_FLIP
#define DISPLAY_FLIP 0
#endif

// settings for qr code generator
#define QR_VERSION 3     // 29 x 29px
#define QR_SCALEFACTOR 2 // 29 -> 58x < 64px

// settings for curve plotter
#define DISPLAY_WIDTH 128 // Width in pixels of OLED-display, must be 32X
#define DISPLAY_HEIGHT 64 // Height in pixels of OLED-display, must be 64X

extern uint8_t DisplayIsOn, displaybuf[];

void setup_display(int contrast = 0);
void refreshTheDisplay(bool nextPage = false);
void init_display(bool verbose = false);
void shutdown_display(void);
void draw_page(time_t t, bool nextpage);
void dp_printf(uint16_t x, uint16_t y, uint8_t font, uint8_t inv,
               const char *format, ...);
void dp_dump(uint8_t *pBuffer);
void dp_printqr(uint16_t offset_x, uint16_t offset_y, const char *Message);
void oledfillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uint8_t bRender);
void oledScrollBufferHorizontal(uint8_t *buf, const uint16_t width,
                                const uint16_t height, bool left = true);
void oledScrollBufferVertical(uint8_t *buf, const uint16_t width,
                              const uint16_t height, int offset = 0);
int oledDrawPixel(uint8_t *buf, const uint16_t x, const uint16_t y,
                  const uint8_t dot);
void oledPlotCurve(uint16_t count, bool reset);
void oledRescaleBuffer(uint8_t *buf, const int factor);

#endif