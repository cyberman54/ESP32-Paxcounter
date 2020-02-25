#ifndef _SDS011READ_H
#define _SDS011READ_H

#include <SDS011.h>

#define SDCARD_FILE_HEADER_SDS011     ", PM10,PM25"

bool sds011_init();
void sds011_loop();
void sds011_sleep(void);
void sds011_wakeup(void);

#endif // _SDS011READ_H
