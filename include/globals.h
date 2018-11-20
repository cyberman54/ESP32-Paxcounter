#ifndef _GLOBALS_H
#define _GLOBALS_H

// The mother of all embedded development...
#include <Arduino.h>

// std::set for unified array functions
#include <set>
#include <array>
#include <algorithm>

// bits in payloadmask for filtering payload data
#define GPS_DATA (0x01)
#define ALARM_DATA (0x02)
#define MEMS_DATA (0x04)
#define COUNT_DATA (0x08)
#define SENSOR1_DATA (0x10)
#define SENSOR2_DATA (0x20)
#define SENSOR3_DATA (0x40)
#define SENSOR4_DATA (0x80)

// bits in configmask for device runmode control
#define GPS_MODE (0x01)
#define ALARM_MODE (0x02)
#define BEACON_MODE (0x04)
#define UPDATE_MODE (0x08)
#define FILTER_MODE (0x10)
#define ANTENNA_MODE (0x20)
#define BLE_MODE (0x40)
#define SCREEN_MODE (0x80)

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
  uint8_t monitormode;   // 0=disabled, 1=enabled
  uint8_t runmode;       // 0=normal, 1=update
  uint8_t payloadmask;   // bitswitches for payload data
  char version[10];      // Firmware version
} configData_t;

// Struct holding payload for data send queue
typedef struct {
  uint8_t MessageSize;
  uint8_t MessagePort;
  uint8_t Message[PAYLOAD_BUFFER_SIZE];
} MessageBuffer_t;

typedef struct {
  uint32_t latitude;
  uint32_t longitude;
  uint8_t satellites;
  uint16_t hdop;
  uint16_t altitude;
} gpsStatus_t;

typedef struct {
  float temperature;       // Temperature in degrees Centigrade
  uint16_t pressure;       // Barometic pressure in hecto pascals
  float humidity;          // Relative humidity in percent
  uint16_t gas_resistance; // Resistance in MOhms
} bmeStatus_t;

// global variables
extern configData_t cfg;                      // current device configuration
extern char display_line6[], display_line7[]; // screen buffers
extern uint8_t volatile channel;              // wifi channel rotation counter
extern uint16_t volatile macs_total, macs_wifi, macs_ble,
    batt_voltage;               // display values
extern std::set<uint16_t> macs; // temp storage for MACs
extern hw_timer_t *channelSwitch, *sendCycle;

extern std::array<uint64_t, 0xff>::iterator it;
extern std::array<uint64_t, 0xff> beacons;

extern TaskHandle_t irqHandlerTask, wifiSwitchTask;

#include "led.h"
#include "payload.h"

#ifdef HAS_GPS
#include "gpsread.h"
#endif

#ifdef HAS_BME
#include "bme680.h"
#endif

#ifdef HAS_LORA
#include "lorawan.h"
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

#ifdef HAS_SENSORS
#include "sensor.h"
#endif

#endif