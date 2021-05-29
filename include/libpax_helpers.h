#ifndef _LIBPAX_HELPERS_H
#define _LIBPAX_HELPERS_H

#include <stdio.h>
#include <libpax_api.h>
#include "senddata.h"
#include "configmanager.h"

void init_libpax(void);

extern uint16_t volatile libpax_macs_ble, libpax_macs_wifi; // libpax values

#endif