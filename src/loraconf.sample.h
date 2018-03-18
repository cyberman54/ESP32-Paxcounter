/************************************************************
 * LMIC LoRaWAN configuration
 * 
 * Read the values from TTN console (or whatever applies)
 * 
 ************************************************************/

#include <Arduino.h>

/* 

// Only define DEVEUI here if you don't want it to be derived from device's MAC address
// Use little endian format (lsb)

//static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5, 0xB3, 0x70 };

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

*/