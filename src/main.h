
#include "configmanager.h"
#include "lorawan.h"
#include "macsniff.h"
#include "senddata.h"

// program version - note: increment version after modifications to configData_t
// struct!!
#define PROGVERSION "1.3.9" // use max 10 chars here!
#define PROGNAME "PAXCNT"

//--- Declarations ---

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

enum led_states { LED_OFF, LED_ON };

#if defined(CFG_eu868)
const char lora_datarate[] = {"1211100908077BFSNA"};
#elif defined(CFG_us915)
const char lora_datarate[] = {"100908078CNA121110090807"};
#endif

//--- Prototypes ---

// defined in main.cpp
void reset_counters(void);
void blink_LED(uint16_t set_color, uint16_t set_blinkduration);
void led_loop(void);

// defined in blescan.cpp
#ifdef BLECOUNTER
void start_BLEscan(void);
void stop_BLEscan(void);
#endif

// defined in gpsread.cpp
#ifdef HAS_GPS
void gps_read(void);
void gps_loop(void *pvParameters);
#endif