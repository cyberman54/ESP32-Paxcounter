#ifndef _DCF77_H
#define _DCF77_H

#include "globals.h"
#include "timekeeper.h"

#ifdef DCF77_ACTIVE_LOW
enum dcf_pinstate { dcf_high, dcf_low };
#else
enum dcf_pinstate { dcf_low, dcf_high };
#endif

enum DCF77_Pulses { dcf_Z, dcf_0, dcf_1 };

void DCF77_Pulse(uint8_t const bit);
void DCF77_Frame(const struct tm t, uint8_t *frame);
uint8_t dec2bcd(uint8_t const dec, uint8_t const startpos, uint8_t const endpos,
                uint8_t *frame);
uint8_t setParityBit(uint8_t const p);

#endif