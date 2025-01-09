#ifndef _MQTTHANDLER_H
#define _MQTTHANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "globals.h"
#include "esp_log.h"

// Function declarations
void pax_mqtt_init(void);
void pax_mqtt_loop(void);
void pax_mqtt_enqueue(uint16_t pax, uint16_t wifi_count, uint16_t ble_count);
void pax_mqtt_connect(void);

extern volatile bool shouldSendMQTT;

#endif