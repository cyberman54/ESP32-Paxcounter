#ifndef _MOBALINE_H
#define _MOBALINE_H

#include "globals.h"
#include "dcf77.h"

#define MOBALINE_FRAME_SIZE (33)
#define MOBALINE_PULSE_LENGTH (100)
#define MOBALINE_HEAD_PULSE_LENGTH (1500)

void MOBALINE_Pulse(time_t t, uint8_t const *DCFpulse);
uint8_t *IRAM_ATTR MOBALINE_Frame(time_t const t);
void IRAM_ATTR dec2bcd(uint8_t const dec, uint8_t const startpos,
                       uint8_t const endpos, uint8_t *DCFpulse);

#endif