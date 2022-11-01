#ifndef _SDCARD_H
#define _SDCARD_H

#if (HAS_SDCARD)

#include "globals.h"
#include <stdio.h>
#include <SPI.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"

#if HAS_SDCARD == 1 // SPI interface
#include "driver/sdspi_host.h"

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

#elif HAS_SDCARD == 2 // MMC interface
#include "driver/sdmmc_host.h"

#ifndef SDCARD_SLOTCONFIG
#define SDCARD_SLOTCONFIG SDMMC_SLOT_CONFIG_DEFAULT()
#endif

#ifndef SDCARD_SLOTWIDTH
#define SDCARD_SLOTWIDTH 1
#endif

#ifndef SDCARD_PULLUP
#define SDCARD_PULLUP SDMMC_SLOT_FLAG_INTERNAL_PULLUP
#endif

#else
#error HAS_SDCARD unknown card reader value, must be either 1 or 2
#endif

#ifdef HAS_SDS011
#include "sds011read.h"
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
