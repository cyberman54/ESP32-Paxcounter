#ifndef _SDS011READ_H
#define _SDS011READ_H

#if (HAS_SDS011)

#include <SdsDustSensor.h>
#include "globals.h"

#define SDCARD_FILE_HEADER_SDS011 ", PM10,PM25"

// use original pins from HardwareSerial if none defined
#ifndef SDS_TX
#define SDS_TX -1
#endif
#ifndef SDS_RX
#define SDS_RX -1
#endif

extern bool isSDS011Active;

bool sds011_init();
void sds011_loop();
void sds011_sleep(void);
void sds011_wakeup(void);
void sds011_store(sdsStatus_t *sds_store);

#endif

#endif // _SDS011READ_H
