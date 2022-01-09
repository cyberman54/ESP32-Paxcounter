#ifndef _MAIN_H
#define _MAIN_H

#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes
#include <esp_event.h>       // needed for Wifi event handler
#include <esp32-hal-timer.h> // needed for timers
#include <esp_coexist.h>     // needed for coex version display
#include <esp_wifi.h>        // needed for wifi init / deinit

#include "globals.h"
#include "reset.h"
#include "i2c.h"
#include "configmanager.h"
#include "cyclic.h"
#include "ota.h"
#include "irqhandler.h"
#include "spislave.h"
#include "sensor.h"
#include "lorawan.h"
#include "timekeeper.h"
#include "corona.h"
#include "boot.h"
#include "libpax_helpers.h"
#include "power.h"
#include "antenna.h"
#include "ntp.h"

#endif