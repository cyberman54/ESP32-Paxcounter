#ifdef HAS_MQTT

#include "mqttclient.h"

static const char TAG[] = __FILE__;

IPAddress mqtt_server_ip(192, 168, 11, 57);

QueueHandle_t MQTTSendQueue;
TaskHandle_t mqttTask;

WiFiClient ipClient;
PubSubClient client(ipClient);

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
  case SYSTEM_EVENT_ETH_START:
    ESP_LOGI(TAG, "ETH Started");
    ETH.setHostname(MQTT_NAME);
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    ESP_LOGI(TAG, "ETH Connected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    ESP_LOGI(TAG, "ETH MAC: %s", ETH.macAddress());
    ESP_LOGI(TAG, "IPv4: %s", ETH.localIP());
    ESP_LOGI(TAG, "Link Speed %d Mbps %s", ETH.linkSpeed(),
             ETH.fullDuplex() ? "full duplex" : "half duplex");
    mqtt_connect(mqtt_server_ip, MQTT_PORT);
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    ESP_LOGI(TAG, "ETH Disconnected");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    ESP_LOGI(TAG, "ETH Stopped");
    break;
  default:
    break;
  }
}

void mqtt_connect(IPAddress mqtt_host, uint16_t mqtt_port) {
  // attempt to connect to MQTT server
  if (ipClient.connect(mqtt_server_ip, MQTT_PORT)) {
    if (client.connect(MQTT_NAME)) {
      ESP_LOGW(TAG, "MQTT server connected, subscribing");
      client.subscribe(MQTT_INTOPIC);
    } else {
      ESP_LOGW(TAG, "MQTT server not responding, retrying later");
    }
  } else
    ESP_LOGW(TAG, "MQTT server not connected, retrying later");
}

void mqtt_client_task(void *param) {
  while (1) {
    MessageBuffer_t msg;
    char cPort[4], cMsg[PAYLOAD_BUFFER_SIZE + 1];

    // fetch next or wait for payload to send from queue
    if (xQueueReceive(MQTTSendQueue, &msg, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // send data
    if (client.connected()) {
      snprintf(cPort, sizeof(cPort), "%d", msg.MessagePort);
      snprintf(cMsg, sizeof(cMsg), "%s", msg.Message);
      client.publish(cPort, cMsg);
      client.loop();
      ESP_LOGI(TAG, "%d byte(s) sent to MQTT", msg.MessageSize);
    } else {
      mqtt_enqueuedata(&msg); // re-enqueue the undelivered message
      delay(10000);
      // attempt to reconnect to MQTT server
      mqtt_connect(mqtt_server_ip, MQTT_PORT);
    }
  }
}

esp_err_t mqtt_init(void) {
  assert(SEND_QUEUE_SIZE);
  MQTTSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (MQTTSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create MQTT send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "MQTT send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);

  WiFi.onEvent(WiFiEvent);
  assert(ETH.begin());

  client.setServer(mqtt_server_ip, MQTT_PORT);
  client.setCallback(mqtt_callback);

  ESP_LOGI(TAG, "Starting MQTTloop...");
  xTaskCreate(mqtt_client_task, "mqttloop", 4096, (void *)NULL, 2, &mqttTask);

  return ESP_OK;
}

void mqtt_enqueuedata(MessageBuffer_t *message) {
  // enqueue message in MQTT send queue
  BaseType_t ret;
  MessageBuffer_t DummyBuffer;
  sendprio_t prio = message->MessagePrio;

  switch (prio) {
  case prio_high:
    // clear space in queue if full, then fallthrough to normal
    if (!uxQueueSpacesAvailable(MQTTSendQueue))
      xQueueReceive(MQTTSendQueue, &DummyBuffer, (TickType_t)0);
  case prio_normal:
    ret = xQueueSendToFront(MQTTSendQueue, (void *)message, (TickType_t)0);
    break;
  case prio_low:
  default:
    ret = xQueueSendToBack(MQTTSendQueue, (void *)message, (TickType_t)0);
    break;
  }
  if (ret != pdTRUE)
    ESP_LOGW(TAG, "MQTT sendqueue is full");
}

void mqtt_queuereset(void) { xQueueReset(MQTTSendQueue); }

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  if ((length) && (topic == MQTT_INTOPIC))
    rcommand(payload, length);
}

#endif // HAS_MQTT