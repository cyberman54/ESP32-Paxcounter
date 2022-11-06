#ifndef _BUTTON_H
#define _BUTTON_H

#include <OneButton.h>
#include "irqhandler.h"
#include "senddata.h"
#include "display.h"
#include "payload.h"

#ifndef BUTTON_ACTIVEHIGH
#define BUTTON_ACTIVEHIGH 0
#endif

#ifndef BUTTON_PULLUP
#define BUTTON_PULLUP 0
#endif

extern TaskHandle_t buttonLoopTask;

void button_init(void);
void IRAM_ATTR readButton(void);

#endif