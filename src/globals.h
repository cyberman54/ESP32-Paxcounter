// The mother of all embedded development...
#include <Arduino.h>

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>

// OLED Display
#ifdef HAS_DISPLAY
#include <U8x8lib.h>
#endif

// GPS
#ifdef HAS_GPS
#include <TinyGPS++.h>
#endif

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>

// LED controls
#ifdef HAS_RGB_LED
#include <SmartLeds.h>
#endif

#include "rgb_led.h"
#include "macsniff.h"
#include "main.h"
#include "payload.h"

extern configData_t cfg;
extern uint64_t uptimecounter;
extern osjob_t sendjob, rcmdjob;
extern char display_lora[], display_lmic[];
extern int countermode, screensaver, adrmode, lorasf, txpower, rlim;
extern uint16_t macs_total, macs_wifi, macs_ble; // MAC counters
extern std::set<uint16_t> macs;
extern hw_timer_t
    *channelSwitch; // hardware timer used for wifi channel switching

#ifdef HAS_GPS
extern gpsStatus_t gps_status; // struct for storing gps data
extern TinyGPSPlus gps;        // Make TinyGPS++ instance globally availabe
#endif

// payload encoder
#if (PAYLOAD_ENCODER == 1)
extern TTNplain payload;
#elif (PAYLOAD_ENCODER == 2)
extern TTNserialized payload;
#elif (PAYLOAD_ENCODER == 3)
//extern CayenneLPP payload;
#else
#error "No valid payload converter defined"
#endif