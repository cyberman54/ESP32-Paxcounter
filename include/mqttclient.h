#ifndef _MQTTCLIENT_H
#define _MQTTCLIENT_H

#include "globals.h"
#include "rcommand.h"
#include <MQTT.h>
#include <ETH.h>

#define MQTT_ETHERNET 0 // select PHY: set 0 for Wifi, 1 for ethernet
#define MQTT_INTOPIC "paxin"
#define MQTT_OUTTOPIC "paxout"
#define MQTT_PORT 1883
#define MQTT_SERVER "paxcounter.cloud.shiftr.io"
#define MQTT_USER "public"
#define MQTT_PASSWD "public"
#define MQTT_RETRYSEC 20  // retry reconnect every 20 seconds
#define MQTT_KEEPALIVE 10 // keep alive interval in seconds

#ifndef MQTT_CLIENTNAME
#define MQTT_CLIENTNAME clientId
#endif

extern TaskHandle_t mqttTask;

void mqtt_enqueuedata(MessageBuffer_t *message);
uint32_t mqtt_queuewaiting(void);
void mqtt_queuereset(void);
void mqtt_client_task(void *param);
int mqtt_connect(const char *my_host, const uint16_t my_port);
void mqtt_callback(MQTTClient *client, char topic[], char payload[],
                   int length);
void NetworkEvent(WiFiEvent_t event);
esp_err_t mqtt_init(void);
void mqtt_deinit(void);

#endif // _MQTTCLIENT_H