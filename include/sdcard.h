#ifndef _SDCARD_H
#define _SDCARD_H

#include <globals.h>
#include <stdio.h>
#include <SPI.h>

#ifdef HAS_SDCARD
#if HAS_SDCARD == 1
#include <mySD.h>
//#include <SD.h>
#elif HAS_SDCARD == 2
#include <SD_MMC.h>
#else
#error HAS_SDCARD unknown card reader value, must be either 1 or 2
#endif
#endif

#ifdef HAS_SDS011
#include "sds011read.h"
#endif

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
static void createFile(void);

#endif
