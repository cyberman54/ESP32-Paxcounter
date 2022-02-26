#ifndef _SDCARD_H
#define _SDCARD_H

#if (HAS_SDCARD)

#include "globals.h"
#include <stdio.h>
#include <SPI.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"

#if HAS_SDCARD == 1
#include "driver/sdspi_host.h"
#elif HAS_SDCARD == 2
#include "driver/sdmmc_host.h"
#else
#error HAS_SDCARD unknown card reader value, must be either 1 or 2
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

#define SDCARD_FILE_NAME clientId
#define SDCARD_FILE_HEADER "timestamp,wifi,ble"

#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
#define SDCARD_FILE_HEADER_VOLTAGE ",voltage"
#endif

bool sdcard_init(bool create = true);
void sdcard_flush(void);
void sdcard_close(void);
void sdcardWriteData(uint16_t, uint16_t, uint16_t = 0);

#endif

#endif // _SDCARD_H
