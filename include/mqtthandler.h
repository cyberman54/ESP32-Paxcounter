#ifndef _MQTTHANDLER_H
#define _MQTTHANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>

// Structure for probe data
struct ProbeData {
    uint32_t pax;
    uint32_t wifi_count;
    uint32_t ble_count;
    uint32_t timestamp;
};

// Structure for device detection data
struct DeviceData {
    uint8_t mac[6];
    int8_t rssi;
    bool is_wifi;  // true for WiFi, false for BLE
    uint32_t timestamp;
};

// Function declarations
void pax_mqtt_init();
void pax_mqtt_loop();
void pax_mqtt_enqueue(uint32_t pax_count, uint32_t wifi_count, uint32_t ble_count);
void pax_mqtt_enqueue_device(const uint8_t* mac, int8_t rssi, bool is_wifi);
void pax_mqtt_send_data();
void pax_mqtt_send_devices();
bool pax_mqtt_connect();

// Hook function for WiFi sniffer
void wifi_packet_handler_hook(uint8_t* mac, int8_t rssi);

#endif