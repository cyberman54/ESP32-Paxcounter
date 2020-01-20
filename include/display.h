#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "cyclic.h"
#include "qrcode.h"

extern uint8_t DisplayIsOn, displaybuf[];

void refreshTheDisplay(bool nextPage = false);
void init_display(bool verbose = false);
void shutdown_display(void);
void draw_page(time_t t, bool nextpage);
void dp_printf(uint16_t x, uint16_t y, uint8_t font, uint8_t inv,
               const char *format, ...);
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