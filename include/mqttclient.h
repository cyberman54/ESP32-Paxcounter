#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <ETH.h>
#include <MQTT.h>

#define MQTT_INTOPIC "paxcounter/in"
#define MQTT_OUTTOPIC "paxcounter/out"
#define MQTT_PORT 1883
#define MQTT_SERVER "broker.shiftr.io"
//#define MQTT_CLIENTNAME "arduino"
#define MQTT_USER "try"
#define MQTT_PASSWD "try"
#define MQTT_RETRYSEC 20  // retry reconnect every 20 seconds
#define MQTT_KEEPALIVE 10 // keep alive interval in seconds
#define MQTT_TIMEOUT 1000 // timeout for all mqtt commands in milliseconds

#ifndef MQTT_CLIENTNAME
#define MQTT_CLIENTNAME clientId.c_str()
#endif

extern TaskHandle_t mqttTask;

void mqtt_enqueuedata(MessageBuffer_t *message);
void mqtt_queuereset(void);
void mqtt_irq(void);
void mqtt_loop(void);
void mqtt_client_task(void *param);
int mqtt_connect(const char *my_host, const uint16_t my_port);
void mqtt_callback(MQTTClient *client, char topic[], char payload[],
                   int length);
void NetworkEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);

#endif // _MQTTCLIENT_H