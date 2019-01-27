#ifndef _IF482_H
#define _IF482_H

#include "globals.h"
#include "irqhandler.h"

extern TaskHandle_t IF482Task;

int if482_init(void);
void if482_loop(void *pvParameters);
void IRAM_ATTR IF482IRQ(void);

#endif