#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <U8x8lib.h>
#include "cyclic.h"

extern uint8_t DisplayState;
extern char timeSource;

extern HAS_DISPLAY u8x8;

void init_display(const char *Productname, const char *Version);
void refreshtheDisplay(void);
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb);

#endif