// COUNTRY AND PROJECT SPECIFIC DEFINITIONS FOR LMIC STACK

// workaround for arduino-espressif32 v2.0.0 (see isse #714 @ MCCI_LMIC)
#define hal_init LMICHAL_init

// COUNTRY SETTINGS
// --> please check with you local regulations for ISM band frequency use!

#define CFG_eu868 1 // Europe (high band)
//#define CFG_us915 1 // USA, Canada and South America
//#define CFG_in866 1 // India
//#define CFG_au915 1 // Australia
//#define CFG_as923 1 // Asia
//#define CFG_kr920 1 // Korea

// LMIC LORAWAN STACK SETTINGS
// --> adapt to your device only if necessary

// use interrupts only if LORA_IRQ and LORA_DIO are connected to interrupt
// capable and separate GPIO pins on your board, if not don't enable
// note: this feature can't be used on ESP32 unless PR #556 of MCCI LMIC was merged
//#if (LORA_IRQ) != (LORA_IO1)
//#define LMIC_USE_INTERRUPTS 1
//#endif

// ensure that a radio is defined.
#if !(defined(CFG_sx1272_radio) || defined(CFG_sx1276_radio) || defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
# warning Target radio not defined, assuming CFG_sx1276_radio
#define CFG_sx1276_radio 1
#elif defined(CFG_sx1272_radio) && (defined(CFG_sx1276_radio) || defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
# error You can define at most one target radio
#elif defined(CFG_sx1276_radio) && (defined(CFG_sx1261_radio) || defined(CFG_sx1262_radio))
# error You can define at most one target radio
#elif defined(CFG_sx1261_radio) && (defined(CFG_sx1261_radio))
# error You can define at most one target radio
#endif

// time sync via LoRaWAN network, note: not supported by TTNv2
#define LMIC_ENABLE_DeviceTimeReq 1

// This tells LMIC to make the receive windows bigger, in case your clock is
// faster or slower. This causes the transceiver to be earlier switched on,
// so consuming more power. You may sharpen (reduce) this value if you are
// limited on battery.
// ATTN: VALUES > 7 WILL CAUSE RECEPTION AND JOIN PROBLEMS WITH HIGH SF RATES
//#define CLOCK_ERROR_PROCENTAGE 5

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

// Change the SPI clock speed if you encounter errors
// communicating with the radio.
// The standard range is 125kHz-8MHz, but some boards can go faster.
//#define LMIC_SPI_FREQ 1E6

// Any runtime assertion failures are printed to this serial port (or
// any other Print object). If this is unset, any failures just silently
// halt execution.
#define LMIC_FAILURE_TO Serial

// Uncomment this to disable all code related to joining
//#define DISABLE_JOIN
// Uncomment this to disable all code related to ping
#define DISABLE_PING
// Uncomment this to disable all code related to beacon tracking.
// Requires ping to be disabled too#define DISABLE_BEACONS

// Uncomment these to disable the corresponding MAC commands.
// Class A
//#define DISABLE_MCMD_DCAP_REQ // duty cycle cap
//#define DISABLE_MCMD_DN2P_SET // 2nd DN window param
//#define DISABLE_MCMD_SNCH_REQ // set new channel
// Class B
//#define DISABLE_MCMD_PING_SET // set ping freq, automatically disabled by
// DISABLE_PING #define DISABLE_MCMD_BCNI_ANS // next beacon start, automatical
// disabled by DISABLE_BEACON

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
//#define USE_ORIGINAL_AES
//
// This selects the AES implementation written by Ideetroon for their
// own LoRaWAN library. It also uses lookup tables, but smaller
// byte-oriented ones, making it use a lot less flash space (but it is
// also about twice as slow as the original).
#define USE_IDEETRON_AES
//
//#define USE_MBEDTLS_AES

// Define this for devices with external power.
//#define LMIC_MCMD_DEVS_BATT_DEFAULT MCMD_DEVS_EXT_POWER
