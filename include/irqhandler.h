#ifndef _IRQHANDLER_H
#define _IRQHANDLER_H

#define DISPLAY_IRQ 0x01
#define BUTTON_IRQ 0x02
#define SENDCYCLE_IRQ 0x04
#define CYCLIC_IRQ 0x08
#define TIMESYNC_IRQ 0x10

#include "globals.h"
#include "cyclic.h"
#include "senddata.h"
#include "timekeeper.h"

void irqHandler(void *pvParameters);

#if(HAS_DISPLAY)
#include "display.h"
void IRAM_ATTR DisplayIRQ();
#endif

#ifdef HAS_BUTTON
#include "button.h"
void IRAM_ATTR ButtonIRQ();
#endif

#endif