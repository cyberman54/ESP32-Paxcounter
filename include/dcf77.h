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

enum DCF77_Pulses { dcf_Z, dcf_0, dcf_1 };

void DCF77_Pulse(time_t t, uint8_t const *DCFpulse);
uint8_t *IRAM_ATTR DCF77_Frame(time_t const t);
uint8_t IRAM_ATTR dec2bcd(uint8_t const dec, uint8_t const startpos, uint8_t const endpos,
                          uint8_t *DCFpulse);
uint8_t IRAM_ATTR setParityBit(uint8_t const p);

#endif