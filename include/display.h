#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "cyclic.h"
#include "qrcode.h"

extern uint8_t DisplayIsOn;

void refreshTheDisplay(bool nextPage = false);
void init_display(void);
void draw_page(time_t t, uint8_t page);
void dp_printf(int x, int y, int font, int inv, const char *format, ...);
void dp_printqr(int offset_x, int offset_y, const char *Message);
void oledfillRect(int x, int y, int width, int height, int bRender);

#endif