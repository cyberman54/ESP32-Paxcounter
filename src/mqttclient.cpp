#ifdef HAS_MQTT

#include "mqttclient.h"

static const char TAG[] = __FILE__;

QueueHandle_t MQTTSendQueue;
TaskHandle_t mqttTask;

Ticker mqttTimer;
WiFiClient netClient;
MQTTClient mqttClient;

esp_err_t mqtt_init(void) {

  // setup network connection
  WiFi.onEvent(NetworkEvent);
  ETH.begin();
  // WiFi.mode(WIFI_STA);
  // WiFi.begin("SSID", "PASSWORD");

  // setup mqtt client
  mqttClient.begin(MQTT_SERVER, MQTT_PORT, netClient);
  mqttClient.onMessageAdvanced(mqtt_callback);

  assert(SEND_QUEUE_SIZE);
  MQTTSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (MQTTSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create MQTT send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "MQTT send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * PAYLOAD_BUFFER_SIZE);

  ESP_LOGI(TAG, "Starting MQTTloop...");
  mqttTimer.attach(MQTT_KEEPALIVE, setMqttIRQ);
  xTaskCreate(mqtt_client_task, "mqttloop", 4096, (void *)NULL, 1, &mqttTask);

  return ESP_OK;
}

int mqtt_connect(const char *my_host, const uint16_t my_port) {
  IPAddress mqtt_server_ip;

  // static String clientId = "paxcounter-" + ETH.macAddress();
  static String clientId = "paxcounter-" + String(random(0xffff), HEX);

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

void NetworkEvent(WiFiEvent_t event) {
  switch (event) {
  case SYSTEM_EVENT_ETH_START:
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "Network link layer started");
    // ETH.setHostname(ETH.macAddress().c_str());
    break;
  case SYSTEM_EVENT_ETH_STOP:
  case SYSTEM_EVENT_STA_STOP:
    ESP_LOGI(TAG, "Network link layer stopped");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
  case SYSTEM_EVENT_STA_CONNECTED:
    ESP_LOGI(TAG, "Network link connected");
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "Network link disconnected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    ESP_LOGI(TAG, "IP: %s", ETH.localIP().toString().c_str());
    ESP_LOGI(TAG, "Link Speed: %d Mbps %s", ETH.linkSpeed(),
             ETH.fullDuplex() ? "full duplex" : "half duplex");
    mqtt_connect(MQTT_SERVER, MQTT_PORT);
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "IP: %s", WiFi.localIP().toString().c_str());
    mqtt_connect(MQTT_SERVER, MQTT_PORT);
    break;

  default:
    break;
  }
}

void mqtt_client_task(void *param) {

  MessageBuffer_t msg;

  while (1) {

    // fetch next or wait for payload to send from queue
    if (xQueueReceive(MQTTSendQueue, &msg, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // send data to mqtt server, if we are connected
    if (mqttClient.connected()) {

      char buffer[PAYLOAD_BUFFER_SIZE + 3];
      snprintf(buffer, msg.MessageSize + 3, "%s/%u", msg.MessagePort,
               msg.Message);

      if (mqttClient.publish(MQTT_OUTTOPIC, buffer)) {
        ESP_LOGI(TAG, "%d byte(s) sent to MQTT server", msg.MessageSize + 2);
        continue;
      } else {
        mqtt_enqueuedata(&msg); // postpone the undelivered message
        ESP_LOGD(TAG,
                 "Couldn't sent message to MQTT server, message postponed");
      }

    } else {
      // attempt to reconnect to MQTT server
      ESP_LOGD(TAG, "MQTT client reconnecting...");
      ESP_LOGD(TAG, "MQTT last_error = %d / rc = %d", mqttClient.lastError(),
               mqttClient.returnCode());
      mqtt_enqueuedata(&msg); // postpone the undelivered message
      delay(MQTT_RETRYSEC * 1000);
      mqtt_connect(MQTT_SERVER, MQTT_PORT);
    }

  } // while(1)
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

void mqtt_callback(MQTTClient *client, char topic[], char payload[],
                   int length) {
  if (topic == MQTT_INTOPIC)
    rcommand((const uint8_t *)payload, (const uint8_t)length);
}

void mqtt_loop(void) {
  if (!mqttClient.loop())
    ESP_LOGD(TAG, "MQTT last_error = %d / rc = %d", mqttClient.lastError(),
             mqttClient.returnCode());
}

void mqtt_queuereset(void) { xQueueReset(MQTTSendQueue); }
void setMqttIRQ(void) { xTaskNotify(irqHandlerTask, MQTT_IRQ, eSetBits); }

#endif // HAS_MQTT