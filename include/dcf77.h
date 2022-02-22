#ifndef _DCF77_H
#define _DCF77_H

#ifdef HAS_DCF77

#include "globals.h"
#include "timekeeper.h"

#define set_dcfbit(b) (1ULL << (b))

#ifdef DCF77_ACTIVE_LOW
enum dcf_pinstate { dcf_high, dcf_low };
#else
enum dcf_pinstate { dcf_low, dcf_high };
#endif

void DCF77_Pulse(uint8_t bit);
uint64_t DCF77_Frame(const struct tm t);

#endif

#endif