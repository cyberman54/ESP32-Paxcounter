#ifndef _LIBPAX_HELPERS_H
#define _LIBPAX_HELPERS_H

#include "globals.h"
#include <stdio.h>
#include <libpax_api.h>

void init_libpax();

extern uint16_t volatile libpax_macs_ble, libpax_macs_wifi; // libpax values

#endif