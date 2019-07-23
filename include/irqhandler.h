#ifndef _IRQHANDLER_H
#define _IRQHANDLER_H

#define DISPLAY_IRQ 0x001
#define BUTTON_IRQ 0x002
#define SENDCYCLE_IRQ 0x004
#define CYCLIC_IRQ 0x008
#define TIMESYNC_IRQ 0x010
#define MASK_IRQ 0x020
#define UNMASK_IRQ 0x040
#define GPS_IRQ 0x080
#define MATRIX_DISPLAY_IRQ 0x100

#include "globals.h"
#include "cyclic.h"
#include "senddata.h"
#include "timekeeper.h"

void irqHandler(void *pvParameters);
bool mask_user_IRQ();
bool unmask_user_IRQ();

#ifdef HAS_DISPLAY
void IRAM_ATTR DisplayIRQ();
#endif

#ifdef HAS_MATRIX_DISPLAY
void IRAM_ATTR MatrixDisplayIRQ();
#endif

#ifdef HAS_BUTTON
void IRAM_ATTR ButtonIRQ();
#endif

#if (HAS_GPS)
void IRAM_ATTR GpsIRQ();
#endif

#endif