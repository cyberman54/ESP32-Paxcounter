#ifndef _DCF77_H
#define _DCF77_H

#include "globals.h"

extern TaskHandle_t DCF77Task;
extern hw_timer_t *dcfCycle;

enum dcf_pulses { dcf_off, dcf_zero, dcf_one };

int dcf77_init(void);
void dcf77_loop(void *pvParameters);
void IRAM_ATTR DCF77IRQ(void);
void sendDCF77(void);

#endif