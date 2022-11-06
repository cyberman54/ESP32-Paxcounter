#ifndef _IRQHANDLER_H
#define _IRQHANDLER_H

#define DISPLAY_IRQ _bitl(0)
#define BUTTON_IRQ _bitl(1)
#define SENDCYCLE_IRQ _bitl(2)
#define CYCLIC_IRQ _bitl(3)
#define TIMESYNC_IRQ _bitl(4)
#define MASK_IRQ _bitl(5)
#define UNMASK_IRQ _bitl(6)
#define BME_IRQ _bitl(7)
#define MATRIX_DISPLAY_IRQ _bitl(8)
#define PMU_IRQ _bitl(9)

#include "globals.h"
#include "button.h"
#include "cyclic.h"
#include "senddata.h"
#include "timekeeper.h"
#include "bmesensor.h"
#include "power.h"
#include "ledmatrixdisplay.h"

void irqHandler(void *pvParameters);
void mask_user_IRQ();
void unmask_user_IRQ();
void IRAM_ATTR doIRQ(int irq);

extern TaskHandle_t irqHandlerTask;

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ();
#endif

#ifdef HAS_MATRIX_DISPLAY
void IRAM_ATTR MatrixDisplayIRQ();
#endif

#endif