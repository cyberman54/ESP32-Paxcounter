#ifndef _SDCARD_H
#define _SDCARD_H

#include <globals.h>
#include <stdio.h>
#include <SPI.h>

#if HAS_SDCARD == 1
#include <mySD.h>
#elif HAS_SDCARD == 2
#include <SD_MMC.h>
#else
#error HAS_SDCARD unknown card reader value, must be either 1 or 2
#endif

#include "sds011read.h"

#ifndef SDCARD_CS
#define SDCARD_CS SS
#endif

#ifndef SDCARD_MOSI
#define SDCARD_MOSI MOSI
#endif

#ifndef SDCARD_MISO
#define SDCARD_MISO MISO
#endif

#ifndef SDCARD_SCLK
#define SDCARD_SCLK SCK
#endif

#define SDCARD_FILE_NAME "/paxcount.%02d"
#define SDCARD_FILE_HEADER "date, time, wifi, bluet"

bool sdcard_init(void);
void sdcardWriteData(uint16_t, uint16_t);

#endif
