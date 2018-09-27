#ifndef _STATEMACHINE_H
#define _STATEMACHINE_H

#include "globals.h"
#include "led.h"
#include "wifiscan.h"
#include "senddata.h"
#include "cyclic.h"

void stateMachine(void *pvParameters);
void stateMachineInit();

#endif
