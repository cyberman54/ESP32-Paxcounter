#ifndef _I2CSCAN_H
#define _I2CSCAN_H

#include <Arduino.h>
#ifdef HAS_PMU
#include "axp20x.h"
#endif

int i2c_scan(void);
void AXP192_init(void);

#endif