
#include "configmanager.h"
#include "senddata.h"

// Does nothing and avoid any compilation error with I2C
#include <Wire.h>

// ESP32 lib Functions
#include <esp32-hal-log.h>  // needed for ESP_LOGx on arduino framework
#include <esp_event_loop.h> // needed for Wifi event handler
#include <esp_spi_flash.h>  // needed for reading ESP32 chip attributes

#ifdef HAS_LORA
#include "lorawan.h"
#endif

#ifdef HAS_DISPLAY
#include "display.h"
#endif

#ifdef HAS_GPS
#include "gps.h"
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

// program version - note: increment version after modifications to configData_t
// struct!!
#define PROGVERSION "1.3.91" // use max 10 chars here!
#define PROGNAME "PAXCNT"

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

#ifdef HAS_GPS
typedef struct {
  uint32_t latitude;
  uint32_t longitude;
  uint8_t satellites;
  uint16_t hdop;
  uint16_t altitude;
} gpsStatus_t;
extern gpsStatus_t gps_status; // struct for storing gps data
extern TinyGPSPlus gps;        // Make TinyGPS++ instance globally availabe
#endif

void reset_counters(void);
void blink_LED(uint16_t set_color, uint16_t set_blinkduration);
void led_loop(void);
uint64_t uptime();