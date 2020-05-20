#ifdef HAS_MQTT

#include "mqttclient.h"

static const char TAG[] = __FILE__;

QueueHandle_t MQTTSendQueue;
TaskHandle_t mqttTask;

WiFiClient EthClient;
PubSubClient mqttClient(EthClient);

void NetworkEvent(WiFiEvent_t event) {
  switch (event) {
  case SYSTEM_EVENT_ETH_START:
    ESP_LOGI(TAG, "Ethernet link layer started");
    ETH.setHostname(MQTT_CLIENT);
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    ESP_LOGI(TAG, "Network link connected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    ESP_LOGI(TAG, "ETH MAC: %s", ETH.macAddress().c_str());
    ESP_LOGI(TAG, "IPv4: %s", ETH.localIP().toString().c_str());
    ESP_LOGI(TAG, "Link Speed: %d Mbps %s", ETH.linkSpeed(),
             ETH.fullDuplex() ? "full duplex" : "half duplex");
    mqtt_connect(MQTT_SERVER, MQTT_PORT);
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    ESP_LOGI(TAG, "Network link disconnected");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    ESP_LOGI(TAG, "Ethernet link layer stopped");
    break;
  default:
    break;
  }
}

int mqtt_connect(const char *my_host, const uint16_t my_port) {
  IPAddress mqtt_server_ip;

  static String clientId = "paxcounter-" + String(random(0xffff), HEX);
  ESP_LOGI(TAG, "MQTT name is %s", clientId.c_str());

  // resolve server
  if (WiFi.hostByName(my_host, mqtt_server_ip)) {
    ESP_LOGI(TAG, "Attempting to connect to %s [%s]", my_host,
             mqtt_server_ip.toString().c_str());
  } else {
    ESP_LOGI(TAG, "Could not resolve %s", my_host);
    return -1;
  }

  // attempt to connect to MQTT server
  if (EthClient.connect(mqtt_server_ip, my_port)) {

    mqttClient.setServer(mqtt_server_ip, my_port);
    mqttClient.setCallback(mqtt_callback);

    if (mqttClient.connect(clientId.c_str())) {
      ESP_LOGI(TAG, "MQTT server connected, subscribing...");
      mqttClient.publish(MQTT_OUTTOPIC, "hello world");
      mqttClient.subscribe(MQTT_INTOPIC);
      mqttClient.loop();
      ESP_LOGI(TAG, "MQTT topic subscribed");
    } else {
      ESP_LOGW(TAG, "MQTT server not responding, retrying later");
      return -1;
    }
  } else {
    ESP_LOGW(TAG, "MQTT server not connected, retrying later");
    return -1;
  }
}

void mqtt_client_task(void *param) {

  MessageBuffer_t msg;
  char cPort[4], cMsg[PAYLOAD_BUFFER_SIZE + 1];

  while (1) {

    // fetch next or wait for payload to send from queue
    if (xQueueReceive(MQTTSendQueue, &msg, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // send data to mqtt server
    if (mqttClient.connected()) {
      snprintf(cPort, sizeof(cPort), "%d", msg.MessagePort);
      snprintf(cMsg, sizeof(cMsg), "%0X", msg.Message);
      ESP_LOGI(TAG, "Topic=%s | Message=%s", cPort, cMsg);
      mqttClient.publish(cPort, cMsg);
      mqttClient.loop();
      ESP_LOGI(TAG, "%d byte(s) sent to MQTT", msg.MessageSize);
    } else {
      mqtt_enqueuedata(&msg); // re-enqueue the undelivered message
      delay(10000);
      // attempt to reconnect to MQTT server
      mqtt_connect(MQTT_SERVER, MQTT_PORT);
    }
  } // while(1)
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

  ESP_LOGI(TAG, "Starting MQTTloop...");
  xTaskCreate(mqtt_client_task, "mqttloop", 4096, (void *)NULL, 2, &mqttTask);

  WiFi.onEvent(NetworkEvent);
  ETH.begin();

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
  if ((length >= 1) && (topic == MQTT_INTOPIC))
    rcommand(payload, length);
}

#endif // HAS_MQTT