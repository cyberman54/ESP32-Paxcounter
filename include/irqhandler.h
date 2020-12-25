#ifndef _IRQHANDLER_H
#define _IRQHANDLER_H

#define DISPLAY_IRQ _bitl(1)
#define BUTTON_IRQ _bitl(2)
#define SENDCYCLE_IRQ _bitl(3)
#define CYCLIC_IRQ _bitl(4)
#define TIMESYNC_IRQ _bitl(5)
#define MASK_IRQ _bitl(6)
#define UNMASK_IRQ _bitl(7)
#define BME_IRQ _bitl(8)
#define MATRIX_DISPLAY_IRQ _bitl(9)
#define PMU_IRQ _bitl(10)

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
void doIRQ(int irq);

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ();
#endif

#ifdef HAS_MATRIX_DISPLAY
void IRAM_ATTR MatrixDisplayIRQ();
#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ();
#endif

#ifdef HAS_PMU
void IRAM_ATTR PMUIRQ();
#endif

#endif