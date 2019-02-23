#ifndef _DCF77_H
#define _DCF77_H

#include "globals.h"

#define DCF77_FRAME_SIZE (60)
#define DCF77_PULSE_LENGTH (100)

#ifdef DCF77_ACTIVE_LOW
enum dcf_pinstate { dcf_high, dcf_low };
#else
enum dcf_pinstate { dcf_low, dcf_high };
#endif

enum dcf_pulses { dcf_off, dcf_zero, dcf_one };

void DCF_Pulse(time_t t);
uint8_t IRAM_ATTR DCF77_Frame(time_t t);
uint8_t IRAM_ATTR dec2bcd(uint8_t dec, uint8_t startpos, uint8_t endpos,
                          uint8_t pArray[]);
uint8_t IRAM_ATTR setParityBit(uint8_t p);

#endif