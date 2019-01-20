#ifndef _BLESCAN_H
#define _BLESCAN_H

#include "globals.h"
#include "macsniff.h"

// Bluetooth specific includes
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
#include <esp_coexist.h>

void start_BLEscan(void);
void stop_BLEscan(void);

#endif