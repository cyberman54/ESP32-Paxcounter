#ifndef _BUTTON_H
#define _BUTTON_H

#include <SimpleButton.h>
#include "irqhandler.h"
#include "senddata.h"
#include "display.h"
#include "payload.h"

#ifdef HAS_E_PAPER_DISPLAY
#include "e_paper_display.h"
#endif

void button_init(int pin);
void readButton();

#endif