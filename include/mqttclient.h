#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <ETH.h>
#include <PubSubClient.h>

#define MQTT_INTOPIC "paxcounter/in"
#define MQTT_OUTTOPIC "paxcounter/out"
#define MQTT_PORT 1883
#define MQTT_SERVER "broker.hivemq.com"
//#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_RETRYSEC 20  // retry reconnect every 20 seconds
#define MQTT_KEEPALIVE 10 // seconds to keep alive server connections

extern TaskHandle_t mqttTask;

void mqtt_enqueuedata(MessageBuffer_t *message);
void mqtt_queuereset(void);
void mqtt_irq(void);
void mqtt_loop(void);
void mqtt_client_task(void *param);
int mqtt_connect(const char *my_host, const uint16_t my_port);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void NetworkEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);

#endif // _MQTTCLIENT_H