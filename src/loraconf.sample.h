/************************************************************
 * LMIC LoRaWAN configuration
 * 
 * Read the values from TTN console (or whatever applies), insert them here,
 * and rename this file to src/loraconf.h

 * Set your DEVEUI here, if you have one. You can leave this untouched, 
 * then the DEVEUI will be generated during runtime from device's MAC adress
 * and will be displayed on device's screen as well as on serial console.
 * 
 * NOTE: Use MSB format (as displayed in TTN console, so you can cut & paste from there)
 * For TTN, APPEUI in MSB format always starts with 0x70, 0xB3, 0xD5
 *
 * Note: If using a board with Microchip 24AA02E64 Uinique ID for deveui,
 * the DEVEUI will be overwriten by the one contained in the Microchip module
 * 
 ************************************************************/

#include <Arduino.h>

static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const u1_t APPEUI[8]={ 0x70, 0xB3, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const u1_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };