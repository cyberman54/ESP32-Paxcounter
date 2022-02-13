#ifndef _LIBPAX_HELPERS_H
#define _LIBPAX_HELPERS_H

#include <stdio.h>
#include <libpax_api.h>
#include "senddata.h"
#include "configmanager.h"

void init_libpax(void);

extern struct count_payload_t count_from_libpax; // libpax count storage

#endif