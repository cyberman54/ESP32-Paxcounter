#ifndef _SDS011READ_H
#define _SDS011READ_H

#include <SDS011.h>

// used pins on the ESP-side:
#define ESP_PIN_TX 19     // connect to RX on the SDS011
#define ESP_PIN_RX 23     // connect to TX on the SDS011

#define SDCARD_FILE_HEADER_SDS011     ", PM10,PM25"

bool sds011_init();
void sds011_loop();
void sds011_sleep(void);
void sds011_wakeup(void);

#endif // _SDS011READ_H
