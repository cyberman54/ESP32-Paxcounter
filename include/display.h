#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "cyclic.h"

extern uint8_t DisplayIsOn;

void init_display(const char *Productname, const char *Version);
void refreshTheDisplay(bool nextPage = false);
void draw_page(time_t t, uint8_t page);
void dp_printf(int x, int y, int font, int inv, const char *format, ...);

#endif