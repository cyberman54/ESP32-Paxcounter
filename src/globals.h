// The mother of all embedded development...
#include <Arduino.h>

// std::set for unified array functions
#include <set>

// OLED Display
#include <U8x8lib.h>

// LMIC-Arduino LoRaWAN Stack
#include <lmic.h>
#include <hal/hal.h>

// Struct holding devices's runtime configuration
typedef struct {
  int8_t lorasf;                       // 7-12, lora spreadfactor
  int8_t txpower;                      // 2-15, lora tx power
  int8_t adrmode;                      // 0=disabled, 1=enabled
  int8_t screensaver;                  // 0=disabled, 1=enabled
  int8_t screenon;                     // 0=disabled, 1=enabled
  int8_t countermode;                  // 0=cyclic unconfirmed, 1=cumulative, 2=cyclic confirmed
  int16_t rssilimit;                   // threshold for rssilimiter, negative value!
  int8_t wifiscancycle;                // wifi scan cycle [seconds/2]
  int8_t wifichancycle;                // wifi channel switch cycle [seconds/100]
  int8_t blescancycle;                 // BLE scan cycle [seconds]
  int8_t blescan;                      // 0=disabled, 1=enabled
  int8_t wifiant;                      // 0=internal, 1=external (for LoPy/LoPy4)
  char version[10];                    // Firmware version
  } configData_t;

extern configData_t cfg;
extern uint8_t mydata[];
extern uint64_t uptimecounter;
extern osjob_t sendjob;
extern uint16_t macnum, blenum, salt;
extern int countermode, screensaver, adrmode, lorasf, txpower, rlim;
extern bool joinstate;
extern std::set<uint32_t> macs; 

#ifdef HAS_DISPLAY
    extern HAS_DISPLAY u8x8;
#else
    extern U8X8_NULL u8x8;
#endif

#ifdef BLECOUNTER
    extern int scanTime;
#endif