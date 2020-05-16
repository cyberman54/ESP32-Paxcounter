#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <ETH.h>
#include <PubSubClient.h>

#define MQTT_NAME "paxcounter"
#define MQTT_INTOPIC "rcommand"
#define MQTT_PORT 1883

extern TaskHandle_t mqttTask;

void mqtt_enqueuedata(MessageBuffer_t *message);
void mqtt_queuereset(void);
void mqtt_client_task(void *param);
void mqtt_connect(IPAddress mqtt_host, uint16_t mqtt_port);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void WiFiEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);

#endif // _MQTTCLIENT_H