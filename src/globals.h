#ifndef _GLOBALS_H
#define _GLOBALS_H

// The mother of all embedded development...
#include <Arduino.h>

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
  uint8_t monitormode;   // 0=disabled, 1=enabled
  uint8_t runmode;       // 0=normal, 1=update
  char version[10];      // Firmware version
} configData_t;

// Struct holding payload for data send queue
typedef struct {
  uint8_t MessageSize;
  uint8_t MessagePort;
  uint8_t Message[PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;

// global variables
extern configData_t cfg;                      // current device configuration
extern char display_line6[], display_line7[]; // screen buffers
extern uint8_t volatile channel;              // wifi channel rotation counter
extern uint16_t volatile macs_total, macs_wifi, macs_ble,
    batt_voltage;               // display values
extern std::set<uint16_t> macs; // temp storage for MACs
extern hw_timer_t *channelSwitch, *sendCycle;
extern portMUX_TYPE timerMux;
extern volatile uint8_t SendCycleTimerIRQ, HomeCycleIRQ, DisplayTimerIRQ,
    ChannelTimerIRQ, ButtonPressedIRQ;

extern std::array<uint64_t, 0xff>::iterator it;
extern std::array<uint64_t, 0xff> beacons;

extern SemaphoreHandle_t xWifiChannelSwitchSemaphore;

#ifdef HAS_GPS
extern TaskHandle_t GpsTask;
#include "gps.h"
#endif

#ifdef HAS_LED
#include "led.h"
#endif

#include "payload.h"

#ifdef HAS_LORA
extern QueueHandle_t LoraSendQueue;
extern TaskHandle_t LoraTask;
#include "lorawan.h"
#endif

#ifdef HAS_SPI
extern QueueHandle_t SPISendQueue;
#endif

#ifdef HAS_DISPLAY
#include "display.h"
#endif

#ifdef HAS_BUTTON
#include "button.h"
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

void reset_counters(void);
void blink_LED(uint16_t set_color, uint16_t set_blinkduration);
uint64_t uptime();

#endif