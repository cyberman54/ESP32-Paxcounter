#include "configportal.h"
#include <ArduinoJson.h>

static AsyncWebServer portal_server(80);
volatile bool config_portal_active = false;
static unsigned long portal_start_time = 0;

// HTML for the configuration page
const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Paxcounter Configuration</title>
    <style>
        body { font-family: Arial; margin: 20px; }
        .container { max-width: 400px; margin: 0 auto; }
        input[type=text], input[type=password], input[type=number] { 
            width: 100%; 
            padding: 12px 20px; 
            margin: 8px 0; 
            display: inline-block; 
            border: 1px solid #ccc; 
            box-sizing: border-box; 
        }
        button { 
            background-color: #4CAF50; 
            color: white; 
            padding: 14px 20px; 
            margin: 8px 0; 
            border: none; 
            cursor: pointer; 
            width: 100%; 
        }
        button:hover { opacity: 0.8; }
    </style>
</head>
<body>
    <div class="container">
        <h2>ESP32 Paxcounter Configuration</h2>
        <form action="/save" method="POST">
            <h3>WiFi Settings</h3>
            <label for="ssid">WiFi SSID:</label>
            <input type="text" id="ssid" name="ssid" required>
            
            <label for="password">WiFi Password:</label>
            <input type="password" id="password" name="password" required>
            
            <h3>MQTT Settings</h3>
            <label for="mqtt_server">MQTT Server:</label>
            <input type="text" id="mqtt_server" name="mqtt_server" required>
            
            <label for="mqtt_port">MQTT Port:</label>
            <input type="number" id="mqtt_port" name="mqtt_port" value="1883" required>
            
            <label for="mqtt_topic">MQTT Topic:</label>
            <input type="text" id="mqtt_topic" name="mqtt_topic" required>
            
            <button type="submit">Save Configuration</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

void init_config_portal() {
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)) {
        ESP_LOGE(TAG, "An error occurred while mounting SPIFFS");
        return;
    }
    
    // Setup configuration portal endpoints
    portal_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", CONFIG_HTML);
    });
    
    portal_server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        String ssid, password, mqtt_server, mqtt_topic;
        int mqtt_port = 1883;
        
        if(request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        }
        if(request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        if(request->hasParam("mqtt_server", true)) {
            mqtt_server = request->getParam("mqtt_server", true)->value();
        }
        if(request->hasParam("mqtt_port", true)) {
            mqtt_port = request->getParam("mqtt_port", true)->value().toInt();
        }
        if(request->hasParam("mqtt_topic", true)) {
            mqtt_topic = request->getParam("mqtt_topic", true)->value();
        }
        
        save_wifi_config(ssid.c_str(), password.c_str(), mqtt_server.c_str(), 
                        mqtt_port, mqtt_topic.c_str());
        
        request->send(200, "text/plain", "Configuration saved. Device will restart...");
        delay(2000);
        ESP.restart();
    });
}

void start_config_portal() {
    if (!config_portal_active) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
        
        portal_server.begin();
        config_portal_active = true;
        portal_start_time = millis();
        
        ESP_LOGI(TAG, "Configuration portal started at IP: %s", 
                WiFi.softAPIP().toString().c_str());
    }
}

void handle_config_portal() {
    if (config_portal_active) {
        // Check for timeout
        if (millis() - portal_start_time > CONFIG_PORTAL_TIMEOUT * 1000) {
            ESP_LOGI(TAG, "Configuration portal timed out");
            config_portal_active = false;
            WiFi.softAPdisconnect(true);
            ESP.restart();
        }
    }
}

bool is_config_portal_active() {
    return config_portal_active;
}

void save_wifi_config(const char* ssid, const char* password, const char* mqtt_server, 
                    int mqtt_port, const char* mqtt_topic) {
    // Create a file to store the configuration
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        ESP_LOGE(TAG, "Failed to open config file for writing");
        return;
    }
    
    StaticJsonDocument<512> doc;
    doc["wifi_ssid"] = ssid;
    doc["wifi_password"] = password;
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_port"] = mqtt_port;
    doc["mqtt_topic"] = mqtt_topic;
    
    if (serializeJson(doc, configFile) == 0) {
        ESP_LOGE(TAG, "Failed to write to config file");
    }
    configFile.close();
    
    ESP_LOGI(TAG, "Configuration saved successfully");
} 