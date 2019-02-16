#ifndef _DCF77_H
#define _DCF77_H

#include "globals.h"

enum dcf_pulses { dcf_off, dcf_zero, dcf_one };
enum dcf_pinstate { dcf_low, dcf_high };

void DCF_Pulse(time_t startTime);
void DCF77_Frame(time_t t);
void set_DCF77_pin(dcf_pinstate state);
uint8_t dec2bcd(uint8_t dec, uint8_t startpos, uint8_t endpos, uint8_t pArray[]);

#endif