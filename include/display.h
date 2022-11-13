#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <libpax_api.h>
#include <Wire.h>
#include "cyclic.h"
#include "qrcode.h"
#include "power.h"
#include "timekeeper.h"

#if (HAS_DISPLAY) == 1
#include <OneBitDisplay.h>
extern ONE_BIT_DISPLAY *dp;
#elif (HAS_DISPLAY) == 2
#include <bb_spi_lcd.h>
extern BB_SPI_LCD *dp;
#endif

#define DISPLAY_PAGES (7) // number of paxcounter display pages
#define PLOTBUFFERSIZE (MY_DISPLAY_WIDTH * MY_DISPLAY_HEIGHT / 8)

// settings for OLED display library
#if (HAS_DISPLAY) == 1
#define MY_FONT_SMALL FONT_6x8
#define MY_FONT_NORMAL FONT_8x8
#define MY_FONT_LARGE FONT_16x32
#define MY_FONT_STRETCHED FONT_12x16
#define MY_DISPLAY_FIRSTLINE 30

#ifndef MY_DISPLAY_RST
#define MY_DISPLAY_RST NOT_A_PIN
#endif

#ifdef MY_DISPLAY_ADDR
#define OLED_ADDR MY_DISPLAY_ADDR
#else
#define OLED_ADDR -1
#endif

#ifndef OLED_FREQUENCY
#define OLED_FREQUENCY 400000L
#endif

#ifndef MY_DISPLAY_FGCOLOR
#define MY_DISPLAY_FGCOLOR 1 // OLED_WHITE
#endif
#ifndef MY_DISPLAY_BGCOLOR
#define MY_DISPLAY_BGCOLOR 0 // OLED_BLACK
#endif

// settings for TFT display library
#elif (HAS_DISPLAY) == 2

#define MY_FONT_SMALL 2
#define MY_FONT_NORMAL 2
#define MY_FONT_LARGE 2
#define MY_FONT_STRETCHED 2
#define MY_DISPLAY_FIRSTLINE 30

#ifndef TFT_FREQUENCY
#define TFT_FREQUENCY 400000L
#endif

#ifndef MY_DISPLAY_FGCOLOR
#define MY_DISPLAY_FGCOLOR TFT_YELLOW
#endif
#ifndef MY_DISPLAY_BGCOLOR
#define MY_DISPLAY_BGCOLOR TFT_BLACK
#endif

#endif

// setup display hardware type, default is OLED 128x64
#ifndef OLED_TYPE
#define OLED_TYPE OLED_128x64
#endif

#ifndef MY_DISPLAY_FLIP
#define MY_DISPLAY_FLIP 0
#endif

#ifndef MY_DISPLAY_WIDTH
#define MY_DISPLAY_WIDTH 128 // Width in pixels of OLED-display, must be 32X
#endif
#ifndef MY_DISPLAY_HEIGHT
#define MY_DISPLAY_HEIGHT 64 // Height in pixels of OLED-display, must be 64X
#endif

#ifndef MY_DISPLAY_FIRSTLINE
#define MY_DISPLAY_FIRSTLINE 0
#endif

// settings for qr code generator
#define QR_VERSION 3 // 29 x 29px

const uint8_t QR_SCALEFACTOR = (MY_DISPLAY_HEIGHT - 4) / 29; // 4px borderlines

extern uint8_t DisplayIsOn;
extern hw_timer_t *displayIRQ;
extern uint8_t volatile channel; // wifi channel rotation counter

void dp_setup(int contrast = 0);
void dp_refresh(bool nextPage = false);
void dp_init(bool verbose = false);
void dp_shutdown(void);
void dp_setFont(int font, int inv = 0);
void dp_dump(uint8_t *pBuffer = NULL);
void dp_contrast(uint8_t contrast);
void dp_clear(void);
void dp_power(uint8_t screenon);
void dp_printqr(uint16_t offset_x, uint16_t offset_y, const char *Message);
void dp_scrollHorizontal(uint8_t *buf, const uint16_t width,
                         const uint16_t height, bool left = true);
void dp_scrollVertical(uint8_t *buf, const uint16_t width,
                       const uint16_t height, int offset = 0);
int dp_drawPixel(uint8_t *buf, const uint16_t x, const uint16_t y,
                 const uint8_t dot);
void dp_plotCurve(uint16_t count, bool reset);
void dp_rescaleBuffer(uint8_t *buf, const int factor);

#endif