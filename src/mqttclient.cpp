#ifdef HAS_MQTT

#include "mqttclient.h"
#include <base64.h>

static const char TAG[] = __FILE__;

QueueHandle_t MQTTSendQueue;
TaskHandle_t mqttTask;

Ticker mqttTimer;
WiFiClient netClient;
MQTTClient mqttClient;

void mqtt_deinit(void) {
  mqttClient.onMessageAdvanced(NULL);
  mqttClient.disconnect();
  vTaskDelete(mqttTask);
}

esp_err_t mqtt_init(void) {

  // setup network connection and MQTT client
  ETH.begin();
  mqttClient.begin(MQTT_SERVER, MQTT_PORT, netClient);
  mqttClient.onMessageAdvanced(mqtt_callback);

  _ASSERT(SEND_QUEUE_SIZE > 0);
  MQTTSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (MQTTSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create MQTT send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "MQTT send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);

  ESP_LOGI(TAG, "Starting MQTTloop...");
  xTaskCreatePinnedToCore(mqtt_client_task, "mqttloop", 4096, (void *)NULL, 1,
                          &mqttTask, 1);
  return ESP_OK;
}

int mqtt_connect(const char *my_host, const uint16_t my_port) {
  IPAddress mqtt_server_ip;
  uint8_t mac[6];
  char clientId[20];

  // hash 6 byte MAC to 4 byte hash
  esp_eth_get_mac(mac);
  const uint32_t hashedmac = hash((const char *)mac, 6);
  snprintf(clientId, 20, "paxcounter_%08x", hashedmac);

  ESP_LOGI(TAG, "MQTT name is %s", MQTT_CLIENTNAME);

  // resolve server host name
  if (WiFi.hostByName(my_host, mqtt_server_ip)) {
    ESP_LOGI(TAG, "Attempting to connect to %s [%s]", my_host,
             mqtt_server_ip.toString().c_str());
  } else {
    ESP_LOGI(TAG, "Could not resolve %s", my_host);
    return -1;
  }

  if (mqttClient.connect(MQTT_CLIENTNAME, MQTT_USER, MQTT_PASSWD)) {
    ESP_LOGI(TAG, "MQTT server connected, subscribing...");
    mqttClient.publish(MQTT_OUTTOPIC, MQTT_CLIENTNAME);
    // Clear retained messages that may have been published earlier on topic
    mqttClient.publish(MQTT_INTOPIC, "", true, 1);
    mqttClient.subscribe(MQTT_INTOPIC);
    ESP_LOGI(TAG, "MQTT topic subscribed");
  } else {
    ESP_LOGD(TAG, "MQTT last_error = %d / rc = %d", mqttClient.lastError(),
             mqttClient.returnCode());
    ESP_LOGW(TAG, "MQTT server not responding, retrying later");
    return -1;
  }

  return 0;
}

void mqtt_client_task(void *param) {

  MessageBuffer_t msg;

  while (1) {

    if (mqttClient.connected()) {

      // check for incoming messages
      mqttClient.loop();

      // fetch next or wait for payload to send from queue
      // do not delete item from queue until it is transmitted
      // consider mqtt timeout while waiting
      if (xQueuePeek(MQTTSendQueue, &msg,
                     MQTT_KEEPALIVE * 1000 / portTICK_PERIOD_MS) != pdTRUE)
        continue;

      // prepare mqtt topic
      char topic[16];
      snprintf(topic, 16, "%s/%u", MQTT_OUTTOPIC, msg.MessagePort);

      // send base64 encoded message to mqtt server and delete it from queue
      if (mqttClient.publish(topic,
                             base64::encode(msg.Message, msg.MessageSize))) {
        ESP_LOGD(TAG, "%s/%s sent to MQTT server", topic,
                 base64::encode(msg.Message, msg.MessageSize));
        xQueueReceive(MQTTSendQueue, &msg, (TickType_t)0);
      } else
        ESP_LOGD(TAG, "Couldn't sent message to MQTT server");
    } else {
      // attempt to reconnect to MQTT server
      ESP_LOGD(TAG, "MQTT client reconnecting...");
      delay(MQTT_RETRYSEC * 1000);
      mqtt_connect(MQTT_SERVER, MQTT_PORT);
    }
  } // while (1)
}

// enqueue outgoing messages in MQTT send queue
void mqtt_enqueuedata(MessageBuffer_t *message) {
  if (xQueueSendToBack(MQTTSendQueue, (void *)message, (TickType_t)0) != pdTRUE)
    ESP_LOGW(TAG, "MQTT sendqueue is full");
}

// process incoming MQTT messages
void mqtt_callback(MQTTClient *client, char topic[], char payload[],
                   int length) {
  if (strcmp(topic, MQTT_INTOPIC) == 0)
    rcommand((const uint8_t *)payload, (const uint8_t)length);
}

void mqtt_queuereset(void) { xQueueReset(MQTTSendQueue); }

uint32_t mqtt_queuewaiting(void) {
  return uxQueueMessagesWaiting(MQTTSendQueue);
}

#endif // HAS_MQTT