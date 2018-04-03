// program version - note: increment version after modifications to configData_t struct!!
#define PROGVERSION                     "1.2.88"    // use max 10 chars here!
#define PROGNAME                        "PAXCNT"

// Verbose enables serial output
#define VERBOSE                         1       // comment out to silence the device, for mute use build option

// set this to include BLE counting and vendor filter functions
#define VENDORFILTER                    1       // comment out if you want to count things, not people
#define BLECOUNTER                      1       // comment out if you don't want BLE count

// BLE scan time
#define BLESCANTIME                     10      // [seconds]
#define BLESCANCYCLE                    2       // BLE scan once after each <BLECYCLE> wifi scans

// WiFi Sniffer cycle interval
#define SEND_SECS                       120     // [seconds/2] -> 240 sec.
//#define SEND_SECS                       30    // [seconds/2] -> 60 sec.

// WiFi sniffer config
#define WIFI_CHANNEL_MIN                1       // start channel number where scan begings
#define	WIFI_CHANNEL_MAX                13      // total channel number to scan
#define WIFI_MY_COUNTRY                 "EU"    // select locale for Wifi RF settings
#define	WIFI_CHANNEL_SWITCH_INTERVAL    50      // [seconds/100] -> 0,5 sec.

// Default LoRa Spreadfactor
#define LORASFDEFAULT                   9       // 7 ... 12 SF, according to LoRaWAN specs
#define MAXLORARETRY                    500     // maximum count of TX retries if LoRa busy
#define RCMDPORT                        2       // LoRaWAN Port on which device listenes for remote commands

// Default RGB LED luminosity (in %)
#define RGBLUMINOSITY                   30      // 30%

// LMIC settings
// define hardware independent LMIC settings here, settings of standard library in /lmic/config.h will be ignored
// define hardware specifics settings in platformio.ini as build_flag for hardware environment

// Select frequency band here according to national regulations
#define CFG_eu868 1
//#define CFG_us915 1

// This is the SX1272/SX1273 radio, which is also used on the HopeRF
// RFM92 boards.
//#define CFG_sx1272_radio 1
// This is the SX1276/SX1277/SX1278/SX1279 radio, which is also used on
// the HopeRF RFM95 boards.
//#define CFG_sx1276_radio 1

// 16 μs per tick
// LMIC requires ticks to be 15.5μs - 100 μs long
#define US_PER_OSTICK_EXPONENT 4
#define US_PER_OSTICK (1 << US_PER_OSTICK_EXPONENT)
#define OSTICKS_PER_SEC (1000000 / US_PER_OSTICK)

// Set this to 1 to enable some basic debug output (using printf) about
// RF settings used during transmission and reception. Set to 2 to
// enable more verbose output. Make sure that printf is actually
// configured (e.g. on AVR it is not by default), otherwise using it can
// cause crashing.
//#define LMIC_DEBUG_LEVEL 1

// Enable this to allow using printf() to print to the given serial port
// (or any other Print object). This can be easy for debugging. The
// current implementation only works on AVR, though.
//#define LMIC_PRINTF_TO Serial

// Any runtime assertion failures are printed to this serial port (or
// any other Print object). If this is unset, any failures just silently
// halt execution.
#define LMIC_FAILURE_TO Serial

// Uncomment this to disable all code related to joining
//#define DISABLE_JOIN
// Uncomment this to disable all code related to ping
#define DISABLE_PING
// Uncomment this to disable all code related to beacon tracking.
// Requires ping to be disabled too
#define DISABLE_BEACONS

// Uncomment these to disable the corresponding MAC commands.
// Class A
//#define DISABLE_MCMD_DCAP_REQ // duty cycle cap
//#define DISABLE_MCMD_DN2P_SET // 2nd DN window param
//#define DISABLE_MCMD_SNCH_REQ // set new channel
// Class B
//#define DISABLE_MCMD_PING_SET // set ping freq, automatically disabled by DISABLE_PING
//#define DISABLE_MCMD_BCNI_ANS // next beacon start, automatical disabled by DISABLE_BEACON

// In LoRaWAN, a gateway applies I/Q inversion on TX, and nodes do the
// same on RX. This ensures that gateways can talk to nodes and vice
// versa, but gateways will not hear other gateways and nodes will not
// hear other nodes. By uncommenting this macro, this inversion is
// disabled and this node can hear other nodes. If two nodes both have
// this macro set, they can talk to each other (but they can no longer
// hear gateways). This should probably only be used when debugging
// and/or when talking to the radio directly (e.g. like in the "raw"
// example).
//#define DISABLE_INVERT_IQ_ON_RX

// This allows choosing between multiple included AES implementations.
// Make sure exactly one of these is uncommented.
//
// This selects the original AES implementation included LMIC. This
// implementation is optimized for speed on 32-bit processors using
// fairly big lookup tables, but it takes up big amounts of flash on the
// AVR architecture.
#define USE_ORIGINAL_AES
//
// This selects the AES implementation written by Ideetroon for their
// own LoRaWAN library. It also uses lookup tables, but smaller
// byte-oriented ones, making it use a lot less flash space (but it is
// also about twice as slow as the original).
// #define USE_IDEETRON_AES
