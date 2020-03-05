#ifndef __LORACONF_H__
#define __LORACONF_H__

#if(HAS_LORA)

/************************************************************
 * LMIC LoRaWAN configuration
 *
 * Read the values from TTN console (or whatever applies), insert them here,
 * and rename this file to src/loraconf.h
 *
 * You can configure you PaxCounter to activate via OTAA (recommended) or ABP.
 * In order to use ABP, uncomment (enable) the following line, 
 * otherwise, leave the line commented (disabled).
 * 
 *************************************************************/

//#define LORA_ABP

/************************************************************
 * OTAA configuration
 * 
 * Note that DEVEUI, APPEUI and APPKEY should all be specified in MSB format.
 * (This is different from standard LMIC-Arduino which expects DEVEUI and APPEUI
 * in LSB format.)

 * Set your DEVEUI here, if you have one. You can leave this untouched,
 * then the DEVEUI will be generated during runtime from device's MAC adress
 * and will be displayed on device's screen as well as on serial console.
 *
 * NOTE: Use MSB format (as displayed in TTN console, so you can cut & paste
 * from there)
 * For TTN, APPEUI in MSB format always starts with 0x70, 0xB3, 0xD5
 *
 * Note: If using a board with Microchip 24AA02E64 Uinique ID for deveui,
 * the DEVEUI will be overwriten by the one contained in the Microchip module
 *
 ************************************************************/
#ifndef LORA_ABP

static const u1_t DEVEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const u1_t APPEUI[8] = {0x70, 0xB3, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00};

static const u1_t APPKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            
#endif


/************************************************************
 * ABP configuration (for development)
 * 
 * Get your
 *   - Network Session Key (NWKSKEY)
 *   - App Session Key and your (APPSKEY)
 *   - Device Address (DEVADDR)
 * from TTN console and replace the example values below.
 * 
 * NOTE: Use MSB format (as displayed in TTN console, so you can cut & paste
 * from there)
 *
 ************************************************************/
#ifdef LORA_ABP

// ID of LoRaAlliance assigned Network (for a list, see e.g. here https://www.thethingsnetwork.org/docs/lorawan/prefix-assignments.html)
static const u1_t NETID = 0x13; // TTN

static const u1_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const u1_t APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const u4_t DEVADDR = 0x00000000; // <-- Change this address for every node!

// set additional ABP parameters in loraconf_abp.cpp
void setABPParamaters();

#endif

#endif // HAS_LORA

#endif // __LORACONF_H__