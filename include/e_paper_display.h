#ifndef _E_PAPER_DISPLAY_H
#define _E_PAPER_DISPLAY_H


extern uint8_t E_paper_displayIsOn;

void ePaper_init(bool verbose);
void refresh_ePaperDisplay(bool nextPage = false);
void draw_page();
int calc_x_rightAlignment(String str, int16_t y);

#endif