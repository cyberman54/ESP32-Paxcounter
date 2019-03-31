#ifndef _IF482_H
#define _IF482_H

#include "globals.h"

#define IF482_FRAME_SIZE (17)
#define IF482_SYNC_FIXUP (2) // calibration to fixup processing time [milliseconds]

String IRAM_ATTR IF482_Frame(time_t tt);

#endif