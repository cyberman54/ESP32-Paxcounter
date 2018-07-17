// The mother of all embedded development...
#include <Arduino.h>

// needed for ESP_LOGx on arduino framework
#include <esp32-hal-log.h>  

// attn: increment version after modifications to configData_t truct!
#define PROGVERSION "1.3.91" // use max 10 chars here!
#define PROGNAME "PAXCNT"

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>

// Struct holding devices's runtime configuration
typedef struct {
  uint8_t lorasf;      // 7-12, lora spreadfactor
  uint8_t txpower;     // 2-15, lora tx power
  uint8_t adrmode;     // 0=disabled, 1=enabled
  uint8_t screensaver; // 0=disabled, 1=enabled
  uint8_t screenon;    // 0=disabled, 1=enabled
  uint8_t countermode; // 0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed
  int16_t rssilimit;   // threshold for rssilimiter, negative value!
  uint8_t sendcycle;   // payload send cycle [seconds/2]
  uint8_t wifichancycle; // wifi channel switch cycle [seconds/100]
  uint8_t blescantime;   // BLE scan cycle duration [seconds]
  uint8_t blescan;       // 0=disabled, 1=enabled
  uint8_t wifiant;       // 0=internal, 1=external (for LoPy/LoPy4)
  uint8_t vendorfilter;  // 0=disabled, 1=enabled
  uint8_t rgblum;        // RGB Led luminosity (0..100%)
  uint8_t gpsmode;       // 0=disabled, 1=enabled
  char version[10];      // Firmware version
} configData_t;

extern configData_t cfg;
extern char display_line6[], display_line7[];
extern int countermode, screensaver, adrmode, lorasf, txpower, rlim;
extern uint8_t channel, DisplayState;
extern uint16_t macs_total, macs_wifi, macs_ble; // MAC counters
extern std::set<uint16_t> macs;
extern hw_timer_t *channelSwitch, *sendCycle;
extern portMUX_TYPE timerMux;

#ifdef HAS_LORA
#include "lorawan.h"
#endif

#ifdef HAS_DISPLAY
#include "display.h"
#endif

#ifdef HAS_GPS
#include "gps.h"
typedef struct {
  uint32_t latitude;
  uint32_t longitude;
  uint8_t satellites;
  uint16_t hdop;
  uint16_t altitude;
} gpsStatus_t;
extern gpsStatus_t gps_status; // struct for storing gps data
#endif

#ifdef BLECOUNTER
#include "blescan.h"
#endif

#ifdef HAS_BATTERY_PROBE
#include "battery.h"
#endif

#ifdef HAS_ANTENNA_SWITCH
#include "antenna.h"
#endif

// class for preparing payload data
#include "payload.h"

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


void reset_counters(void);
void blink_LED(uint16_t set_color, uint16_t set_blinkduration);
void led_loop(void);
uint64_t uptime();