#ifndef _SDS011READ_H
#define _SDS011READ_H

#include <SDS011.h>
#include "globals.h"

#define SDCARD_FILE_HEADER_SDS011     ", PM10,PM25"

bool sds011_init();
void sds011_loop();
void sds011_sleep(void);
void sds011_wakeup(void);
void sds011_store(sdsStatus_t *sds_store);

#endif // _SDS011READ_H
