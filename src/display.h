#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <U8x8lib.h>

extern uint8_t volatile DisplayState;
extern HAS_DISPLAY u8x8;

void init_display(const char *Productname, const char *Version);
void refreshtheDisplay(void);
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb);
void DisplayIRQ(void);

#endif