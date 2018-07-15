// The mother of all embedded development...
#include <Arduino.h>

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>

// basics
#include "main.h"
#include "led.h"
#include "macsniff.h"
#include "payload.h"

extern configData_t cfg;
extern char display_line6[], display_line7[];
extern int countermode, screensaver, adrmode, lorasf, txpower, rlim;
extern uint8_t channel, DisplayState;
extern uint16_t macs_total, macs_wifi, macs_ble; // MAC counters
extern uint64_t uptimecounter;
extern std::set<uint16_t> macs;
extern hw_timer_t *channelSwitch, *sendCycle;
extern portMUX_TYPE timerMux;

#if defined(CFG_eu868)
const char lora_datarate[] = {"1211100908077BFSNA"};
#elif defined(CFG_us915)
const char lora_datarate[] = {"100908078CNA121110090807"};
#endif

#ifdef HAS_GPS
extern gpsStatus_t gps_status; // struct for storing gps data
extern TinyGPSPlus gps;        // Make TinyGPS++ instance globally availabe
#endif

// payload encoder
#if PAYLOAD_ENCODER == 1
extern TTNplain payload;
#elif PAYLOAD_ENCODER == 2
extern TTNpacked payload;
#elif PAYLOAD_ENCODER == 3
extern CayenneLPP payload;
#else
#error "No valid payload converter defined"
#endif