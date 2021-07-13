#ifndef _SOFTSERIAL_H
#define _SOFTSERIAL_H

#include <globals.h>
#include <stdio.h>
#include <SPI.h>

bool serial_init(void);
void serialWriteData(uint16_t, uint16_t, uint16_t = 0);

#endif // _SOFTSERIAL_H
