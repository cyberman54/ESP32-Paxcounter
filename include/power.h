#ifndef _POWER_H
#define _POWER_H

#include <Arduino.h>
#include "i2cscan.h"

#ifdef HAS_PMU
#include <axp20x.h>
#endif

void AXP192_init(void);

#endif