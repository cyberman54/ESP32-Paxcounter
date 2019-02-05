#ifndef _DCF77_H
#define _DCF77_H

#include "globals.h"

extern TaskHandle_t DCF77Task;
extern hw_timer_t *dcfCycle;

enum dcf_pulses { dcf_off, dcf_zero, dcf_one };
enum dcf_pinstate { dcf_low, dcf_high };

void IRAM_ATTR DCF77IRQ(void);
int dcf77_init(void);
void dcf77_loop(void *pvParameters);
void sendDCF77(void);
void DCF_Out(uint8_t startsec);
void generateTimeframe(time_t t);
void set_DCF77_pin(dcf_pinstate state);
uint8_t dec2bcd(uint8_t dec, uint8_t startpos, uint8_t endpos, uint8_t pArray[]);

#endif