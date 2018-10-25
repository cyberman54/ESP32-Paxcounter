#ifndef _lmic_config_h_
#define _lmic_config_h_

// In the original LMIC code, these config values were defined on the
// gcc commandline. Since Arduino does not allow easily modifying the
// compiler commandline unless you modify the BSP, you have two choices:
//
//  - edit {libraries}/arduino-lmic/project_config/lmic_project_config.h;
//  - use a BSP like the MCCI Arduino BSPs, which get the configuration
//    from the boards.txt file through a menu option.
//
// You definitely should not edit this file.

// set up preconditions, and load configuration if needed.
#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

// check post-conditions.

// make sure that we have exactly one target region defined.
#if CFG_LMIC_REGION_MASK == 0
# define CFG_eu868 1
#elif (CFG_LMIC_REGION_MASK & (-CFG_LMIC_REGION_MASK)) != CFG_LMIC_REGION_MASK
# error You can define at most one of CFG_... variables
#elif (CFG_LMIC_REGION_MASK & LMIC_REGIONS_SUPPORTED) == 0
# error The selected CFG_... region is not supported yet.
#endif

// make sure that LMIC_COUNTRY_CODE is defined.
#ifndef LMIC_COUNTRY_CODE
# define LMIC_COUNTRY_CODE      LMIC_COUNTRY_CODE_NONE
#endif

// if the country code is Japan, then the region must be AS923
#if LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP && CFG_region != LMIC_REGION_as923
# error "If country code is JP, then region must be AS923"
#endif

// check for internal consistency
#if !(CFG_LMIC_EU_like || CFG_LMIC_US_like)
# error "Internal error: Neither EU-like nor US-like!"
#endif

// This is the SX1272/SX1273 radio, which is also used on the HopeRF
// RFM92 boards.
//#define CFG_sx1272_radio 1
// This is the SX1276/SX1277/SX1278/SX1279 radio, which is also used on
// the HopeRF RFM95 boards.
//#define CFG_sx1276_radio 1

// ensure that a radio is defined.
#if ! (defined(CFG_sx1272_radio) || defined(CFG_sx1276_radio))
# warning Target radio not defined, assuming CFG_sx1276_radio
#define CFG_sx1276_radio 1
#elif defined(CFG_sx1272_radio) && defined(CFG_sx1276_radio)
# error You can define at most one of CFG_sx1272_radio and CF_sx1276_radio
#endif

// LMIC requires ticks to be 15.5μs - 100 μs long
#ifndef OSTICKS_PER_SEC
// 16 μs per tick
# ifndef US_PER_OSTICK_EXPONENT
#   define US_PER_OSTICK_EXPONENT 4
# endif
# define US_PER_OSTICK (1 << US_PER_OSTICK_EXPONENT)
# define OSTICKS_PER_SEC (1000000 / US_PER_OSTICK)
#endif /* OSTICKS_PER_SEC */

#if ! (10000 <= OSTICKS_PER_SEC && OSTICKS_PER_SEC < 64516)
# error LMIC requires ticks to be 15.5 us to 100 us long
#endif

// Change the SPI clock speed if you encounter errors
// communicating with the radio.
// The standard range is 125kHz-8MHz, but some boards can go faster.
#ifndef LMIC_SPI_FREQ
#define LMIC_SPI_FREQ 1E6
#endif

// Set this to 1 to enable some basic debug output (using printf) about
// RF settings used during transmission and reception. Set to 2 to
// enable more verbose output. Make sure that printf is actually
// configured (e.g. on AVR it is not by default), otherwise using it can
// cause crashing.
#ifndef LMIC_DEBUG_LEVEL
#define LMIC_DEBUG_LEVEL 0
#endif

// Enable this to allow using printf() to print to the given serial port
// (or any other Print object). This can be easy for debugging. The
// current implementation only works on AVR, though.
//#define LMIC_PRINTF_TO Serial

// Enable this to use interrupt handler routines listening for RISING signals.
// Otherwise, the library polls digital input lines for changes.
//#define LMIC_USE_INTERRUPTS

// If DISABLE_LMIC_FAILURE_TO is defined, runtime assertion failures
// silently halt execution. Otherwise, LMIC_FAILURE_TO should be defined
// as the name of an object derived from Print, which will be used for
// displaying runtime assertion failures. If you say nothing in your
// lmic_project_config.h, runtime assertion failures are displayed
// using the Serial object.
#if ! defined(DISABLE_LMIC_FAILURE_TO) && ! defined(LMIC_FAILURE_TO)
#define LMIC_FAILURE_TO Serial
#endif

// define this in lmic_project_config.h to disable all code related to joining
//#define DISABLE_JOIN
// define this in lmic_project_config.h to disable all code related to ping
//#define DISABLE_PING
// define this in lmic_project_config.h to disable all code related to beacon tracking.
// Requires ping to be disabled too
//#define DISABLE_BEACONS

// define these in lmic_project_config.h to disable the corresponding MAC commands.
// Class A
//#define DISABLE_MCMD_DCAP_REQ // duty cycle cap
//#define DISABLE_MCMD_DN2P_SET // 2nd DN window param
//#define DISABLE_MCMD_SNCH_REQ // set new channel
// Class B
//#define DISABLE_MCMD_PING_SET // set ping freq, automatically disabled by DISABLE_PING
//#define DISABLE_MCMD_BCNI_ANS // next beacon start, automatically disabled by DISABLE_BEACON

// In LoRaWAN, a gateway applies I/Q inversion on TX, and nodes do the
// same on RX. This ensures that gateways can talk to nodes and vice
// versa, but gateways will not hear other gateways and nodes will not
// hear other nodes. By defining this macro in lmic_project_config.h,
// this inversion is disabled and this node can hear other nodes. If
// two nodes both have this macro set, they can talk to each other
// (but they can no longer hear gateways). This should probably only
// be used when debugging and/or when talking to the radio directly
// (e.g. like in the "raw" example).
//#define DISABLE_INVERT_IQ_ON_RX

// This allows choosing between multiple included AES implementations.
// Make sure exactly one of these is uncommented.
//
// This selects the original AES implementation included LMIC. This
// implementation is optimized for speed on 32-bit processors using
// fairly big lookup tables, but it takes up big amounts of flash on the
// AVR architecture.
// #define USE_ORIGINAL_AES
//
// This selects the AES implementation written by Ideetroon for their
// own LoRaWAN library. It also uses lookup tables, but smaller
// byte-oriented ones, making it use a lot less flash space (but it is
// also about twice as slow as the original).
// #define USE_IDEETRON_AES

#if ! (defined(USE_ORIGINAL_AES) || defined(USE_IDEETRON_AES))
# define USE_IDEETRON_AES
#endif

#if defined(USE_ORIGINAL_AES) && defined(USE_IDEETRON_AES)
# error "You may define at most one of USE_ORIGINAL_AES and USE_IDEETRON_AES"
#endif

// LMIC_DISABLE_DR_LEGACY
// turn off legacy DR_* symbols that vary by bandplan.
// Older code uses these for configuration. EU868_DR_*, US915_DR_*
// etc symbols are prefered, but breaking older code is inconvenient for
// everybody. We don't want to use DR_* in the LMIC itself, so we provide
// this #define to allow them to be removed.
#if !defined(LMIC_DR_LEGACY)
# if !defined(LMIC_DISABLE_DR_LEGACY)
#  define LMIC_DR_LEGACY 1
# else // defined(LMIC_DISABLE_DR_LEGACY)
#  define LMIC_DR_LEGACY 0
# endif // defined(LMIC_DISABLE_DR_LEGACY)
#endif // LMIC_DR_LEGACY

#endif // _lmic_config_h_
