#ifndef _CLOCKCONTROLLER_H
#define _CLOCKCONTROLLER_H

#include "globals.h"

#ifdef HAS_IF482
#include "if482.h"
#elif defined HAS_DCF77
#include "dcf77.h"
#endif

void clock_init(void);
void clock_loop(void *pvParameters);

#endif // _CLOCKCONTROLLER_H