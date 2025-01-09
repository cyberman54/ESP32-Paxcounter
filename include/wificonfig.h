#ifndef _WIFICONFIG_H
#define _WIFICONFIG_H

#include <Arduino.h>
#include "globals.h"

// Include configuration from paxcounter.conf
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER ""
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_OUTTOPIC
#define MQTT_OUTTOPIC "paxcounter/data"
#endif

#ifndef MQTT_SEND_INTERVAL
#define MQTT_SEND_INTERVAL 30
#endif

// Configuration structure
struct WiFiConfig {
    String ssid;
    String password;
    String mqtt_server;
    String mqtt_topic;
    int mqtt_port;

    // Constructor to initialize with default values from paxcounter.conf
    WiFiConfig() : 
        ssid(WIFI_SSID),
        password(WIFI_PASSWORD),
        mqtt_server(MQTT_SERVER),
        mqtt_topic(MQTT_OUTTOPIC),
        mqtt_port(MQTT_PORT)
    {}
};

extern WiFiConfig wifiConfig;

#endif 