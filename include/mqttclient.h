#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <ETH.h>
#include <PubSubClient.h>

#define MQTT_CLIENT "paxcounter"
#define MQTT_INTOPIC "pax_IN"
#define MQTT_OUTTOPIC "pax_OUT"
#define MQTT_PORT 1883
#define MQTT_SERVER "broker.hivemq.com"

extern TaskHandle_t mqttTask;

void mqtt_enqueuedata(MessageBuffer_t *message);
void mqtt_queuereset(void);
void mqtt_client_task(void *param);
int mqtt_connect(IPAddress mqtt_host, uint16_t mqtt_port);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void NetworkEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);

#endif // _MQTTCLIENT_H