#ifndef _IF482_H
#define _IF482_H

#ifdef HAS_IF482

#include "globals.h"
#include "timekeeper.h"
#include "esp_sntp.h"

#define IF482_FRAME_SIZE (17)
#define IF482_SYNC_FIXUP (10) // calibration to fixup processing time [milliseconds]

String IF482_Frame(time_t t);

#endif

#endif