#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <U8x8lib.h>
#include "cyclic.h"

extern uint8_t DisplayIsOn;

extern HAS_DISPLAY u8x8;

void init_display(const char *Productname, const char *Version);
void refreshTheDisplay(bool nextPage = false);
void draw_page(time_t t, uint8_t page);
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb);

#endif