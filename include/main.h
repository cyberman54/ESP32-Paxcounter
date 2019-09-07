#ifndef _MAIN_H
#define _MAIN_H

#include <esp_spi_flash.h>   // needed for reading ESP32 chip attributes
#include <esp_event_loop.h>  // needed for Wifi event handler
#include <esp32-hal-timer.h> // needed for timers
#include <esp_coexist.h>     // needed for showing coex sw version

#include "globals.h"
#include "power.h"
#include "i2cscan.h"
#include "blescan.h"
#include "wifiscan.h"
#include "configmanager.h"
#include "cyclic.h"
#include "beacon_array.h"
#include "ota.h"
#include "irqhandler.h"
#include "led.h"
#include "spislave.h"
#if (HAS_LORA)
#include "lorawan.h"
#endif
#include "timekeeper.h"
#endif
