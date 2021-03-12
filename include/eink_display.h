#ifndef _EINK_DISPLAY_H
#define _EINK_DISPLAY_H


// #include <GxEPD.h>
// #include <GxIO/GxIO_SPI/GxIO_SPI.h>
// #include <GxIO/GxIO.h>
// #include <eink_choose_board.h>






extern uint8_t EInkDisplayIsOn;

void eInk_init(bool verbose);
void refreshEInk_display(bool nextPage = false);
void draw_page();
int calc_x_rightAlignment(String str, int16_t y);

#endif