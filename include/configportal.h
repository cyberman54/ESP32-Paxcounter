#ifndef _CONFIGPORTAL_H
#define _CONFIGPORTAL_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include "globals.h"
#include "configmanager.h"

// Configuration portal settings
#define CONFIG_AP_SSID "ESP32-Paxcounter"
#define CONFIG_AP_PASSWORD "configure123"
#define CONFIG_PORTAL_TIMEOUT 300 // 5 minutes timeout

// Function declarations
void init_config_portal(void);
void start_config_portal(void);
void handle_config_portal(void);
bool is_config_portal_active(void);
void save_wifi_config(const char* ssid, const char* password, const char* mqtt_server, 
                    int mqtt_port, const char* mqtt_topic);

extern volatile bool config_portal_active;

#endif 