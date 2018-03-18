// First things first
#include "main.h"

// The mother of all embedded development...
#include <Arduino.h>

// we neededthis  to ESP_LOGx on arduino framework
#include "esp32-hal-log.h"

#include <String.h>

// std::set for unified array functions
#include <set>

// OLED Display
#include <U8x8lib.h>

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>

// configData_t
#include "configmanager.h"

extern uint8_t mydata[];
extern uint64_t uptimecounter;
extern int macnum, blenum, countermode, screensaver, adrmode, lorasf, txpower, rlim;
extern bool joinstate;

extern osjob_t sendjob;

extern std::set<uint64_t, std::greater <uint64_t> > macs;

extern U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8;

extern configData_t cfg;

#ifdef BLECOUNTER
    extern int scanTime;
#endif