#ifndef _HASH_H
#define _HASH_H

#include <Arduino.h>
#include <inttypes.h>

uint32_t IRAM_ATTR rokkit(const char *data, int len);

#endif