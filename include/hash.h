#ifndef _HASH_H
#define _HASH_H

#include <Arduino.h>
#include <RokkitHash.h>

uint32_t IRAM_ATTR hash(const char *data, int len);

#endif