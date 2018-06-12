
#include "configmanager.h"
#include "lorawan.h"
#include "macsniff.h"

// program version - note: increment version after modifications to configData_t
// struct!!
#define PROGVERSION "1.3.81" // use max 10 chars here!
#define PROGNAME "PAXCNT"

//--- Declarations ---

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
