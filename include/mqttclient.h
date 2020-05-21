#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <ETH.h>
#include <PubSubClient.h>

#define MQTT_INTOPIC "paxcounter_in/"
#define MQTT_OUTTOPIC "paxcounter_out/"
#define MQTT_PORT 1883
#define MQTT_SERVER "broker.hivemq.com"

extern TaskHandle_t mqttTask;
extern PubSubClient mqttClient;

void mqtt_enqueuedata(MessageBuffer_t *message);
void mqtt_queuereset(void);
void mqtt_client_task(void *param);
int mqtt_connect(const char *my_host, const uint16_t my_port);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void NetworkEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);

#endif // _MQTTCLIENT_H