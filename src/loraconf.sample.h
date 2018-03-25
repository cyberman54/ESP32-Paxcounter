/************************************************************
 * LMIC LoRaWAN configuration
 * 
 * Read the values from TTN console (or whatever applies)
 * 
 ************************************************************/

#include <Arduino.h>

/* 

// Set your DEVEUI here, if you have one. You can leave this untouched, 
// then the DEVEUI will be generated during runtime from device's MAC adress
// Note: Use same format as in TTN console (cut & paste, for your convenience)
static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Note: Use msb format for APPEUI as in TTN console (cut & paste, for your convenience)
// For TTN, APPEUI always starts with 0x70, 0xB3, 0xD5
static const u1_t APPEUI[8]={ 0x70, 0xB3, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00 };

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

*/