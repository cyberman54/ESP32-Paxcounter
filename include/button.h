#ifndef _BUTTON_H
#define _BUTTON_H

#include <SimpleButton.h>
#include "irqhandler.h"
#include "senddata.h"
#include "display.h"
#include "payload.h"

#ifndef BUTTON_ACTIVEHIGH
#define BUTTON_ACTIVEHIGH 0
#endif

void button_init(int pin);
void readButton();

#endif