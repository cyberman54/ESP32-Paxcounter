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
// *** Take care : If Using a board with Microchip 24AA02E64 Uinique ID for deveui, **
// *** this DEVEUI will be overwriten by the one contained in the Microchip module ***
#if defined (ttgov2)
static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
#elif defined (ttgov1)
static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
#elif defined (heltec_wifi_lora_32)
static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32 };
#else
static const u1_t DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif

// Note: Use msb format for APPEUI as in TTN console (cut & paste, for your convenience)
// For TTN, APPEUI always starts with 0x70, 0xB3, 0xD5
static const u1_t APPEUI[8]={ 0x70, 0xB3, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Note: Use msb format for APPEUI as in TTN console (cut & paste, for your convenience)
static const u1_t APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

*/