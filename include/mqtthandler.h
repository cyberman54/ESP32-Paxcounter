#ifndef _MQTTHANDLER_H
#define _MQTTHANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <CircularBuffer.hpp>

// MQTT Configuration
#define PAX_MQTT_SERVER "192.168.2.249"
#define PAX_MQTT_PORT 1883
#define PAX_MQTT_KEEPALIVE 60
#define PAX_MQTT_CLIENTNAME "esp32-paxcounter"
#define PAX_MQTT_OUTTOPIC "paxcounter/data"
#define PAX_MQTT_QUEUE_SIZE 100

// Button Configuration
#define PAX_MQTT_TRIGGER_PIN 2
#define PAX_MQTT_TRIGGER_MODE INPUT_PULLUP

// WiFi Configuration Constants
#ifndef WIFI_SSID
#define WIFI_SSID "LastrillaWIFI"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "cp880uegkg"
#endif

// Structure for probe data
struct ProbeData {
    uint32_t pax;
    uint32_t wifi_count;
    uint32_t ble_count;
    uint32_t timestamp;
};

// Function declarations
void pax_mqtt_init();
void pax_mqtt_loop();
void pax_mqtt_enqueue(uint32_t pax_count, uint32_t wifi_count, uint32_t ble_count);
void pax_mqtt_send_data();
bool pax_mqtt_connect();

#endif