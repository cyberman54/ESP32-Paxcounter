#ifndef _WIFISCAN_H
#define _WIFISCAN_H

// ESP32 Functions
#include <esp_wifi.h>

#include "hash.h"    // Hash function for scrambling MAC addresses
#include "antenna.h" // code for switching wifi antennas
#include "macsniff.h"

void wifi_sniffer_init(void);
void switch_wifi_sniffer(uint8_t state);
void IRAM_ATTR wifi_sniffer_packet_handler(void *buff,
                                           wifi_promiscuous_pkt_type_t type);
void switchWifiChannel(TimerHandle_t xTimer);

#endif