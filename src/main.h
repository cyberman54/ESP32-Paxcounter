#ifndef _MAIN_H
#define _MAIN_H

#include "globals.h"
#include "led.h"
#include "macsniff.h"
#include "wifiscan.h"
#include "configmanager.h"
#include "senddata.h"
#include "cyclic.h"
#include "beacon_array.h"
#include "ota.h"

#include <esp_spi_flash.h>  // needed for reading ESP32 chip attributes
#include <esp_event_loop.h> // needed for Wifi event handler

#endif