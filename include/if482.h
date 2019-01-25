#ifndef _IF482_H
#define _IF482_H

#include "globals.h"
#include "irqhandler.h"

void if482_init(void);
void sendIF482(time_t t);

#endif