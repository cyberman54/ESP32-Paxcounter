#ifndef _EINK_DISPLAY_H
#define _EINK_DISPLAY_H


extern uint8_t EInkDisplayIsOn;

void eInk_init(bool verbose);
void refreshEInk_display(bool nextPage = false);
void draw_page();
int calc_x_rightAlignment(String str, int16_t y);

#endif