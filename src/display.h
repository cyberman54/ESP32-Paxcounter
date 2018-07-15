#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8x8lib.h>

void init_display(const char *Productname, const char *Version);
void refreshDisplay(void);
void DisplayKey(const uint8_t *key, uint8_t len, bool lsb);

#endif