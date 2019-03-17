#ifndef _IF482_H
#define _IF482_H

#include "globals.h"
#include "timekeeper.h"

#define IF482_FRAME_SIZE (17)

extern HardwareSerial IF482;

void IF482_Pulse(time_t t);
String IRAM_ATTR IF482_Frame(time_t tt);

#endif