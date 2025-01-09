#include "mqtthandler.h"
#include "wificonfig.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

static const char* MQTT_TAG = "MQTT";

// Global variables
volatile bool shouldSendMQTT = false;

// MQTT client
WiFiClient paxWifiClient;
PubSubClient paxMqttClient(paxWifiClient);

// Queue for button presses
static QueueHandle_t mqttButtonQueue = NULL;

// Timer for periodic MQTT updates
static TimerHandle_t mqttSendTimer = NULL;

void mqttSendTimerCallback(TimerHandle_t xTimer) {
    shouldSendMQTT = true;
}

void pax_mqtt_enqueue(uint16_t pax, uint16_t wifi_count, uint16_t ble_count) {
    StaticJsonDocument<200> doc;
    doc["pax"] = pax;
    doc["wifi"] = wifi_count;
    doc["ble"] = ble_count;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (paxMqttClient.connected()) {
        const char* topic = wifiConfig.mqtt_topic.c_str();
        paxMqttClient.publish(topic, jsonString.c_str());
        ESP_LOGI(MQTT_TAG, "Published to %s: %s", topic, jsonString.c_str());
    }
}

void pax_mqtt_connect() {
    // Load configuration from SPIFFS if not already loaded
    if (SPIFFS.exists("/config.json")) {
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, configFile);
            
            if (!error) {
                wifiConfig.ssid = doc["wifi_ssid"].as<String>();
                wifiConfig.password = doc["wifi_password"].as<String>();
                wifiConfig.mqtt_server = doc["mqtt_server"].as<String>();
                wifiConfig.mqtt_topic = doc["mqtt_topic"].as<String>();
                wifiConfig.mqtt_port = doc["mqtt_port"].as<int>();
                ESP_LOGI(MQTT_TAG, "Loaded configuration from SPIFFS");
            }
            configFile.close();
        }
    }
    
    // Connect to WiFi using available credentials
    WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        ESP_LOGI(MQTT_TAG, "Connecting to WiFi... (%d/20)", attempts + 1);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(MQTT_TAG, "Connected to WiFi");
        
        // Configure MQTT client
        paxMqttClient.setServer(wifiConfig.mqtt_server.c_str(), wifiConfig.mqtt_port);
        
        // Connect to MQTT broker
        if (paxMqttClient.connect(clientId)) {
            ESP_LOGI(MQTT_TAG, "Connected to MQTT broker");
        } else {
            ESP_LOGE(MQTT_TAG, "Failed to connect to MQTT broker");
        }
    } else {
        ESP_LOGE(MQTT_TAG, "Failed to connect to WiFi");
    }
}

void pax_mqtt_loop() {
    static unsigned long lastReconnectAttempt = 0;
    
    if (!paxMqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            pax_mqtt_connect();
        }
    }
    
    paxMqttClient.loop();
}

void pax_mqtt_init() {
    ESP_LOGI(MQTT_TAG, "Initializing MQTT Handler...");
    
    // Create timer for periodic updates
    mqttSendTimer = xTimerCreate(
        "MQTTSendTimer",
        pdMS_TO_TICKS(MQTT_SEND_INTERVAL * 1000),
        pdTRUE,
        (void*)0,
        mqttSendTimerCallback
    );
    
    if (mqttSendTimer != NULL) {
        xTimerStart(mqttSendTimer, 0);
    }
    
    // Initial connection
    pax_mqtt_connect();
    
    ESP_LOGI(MQTT_TAG, "MQTT Handler initialized");
}

// Hook function implementation for WiFi sniffer
void IRAM_ATTR wifi_packet_handler_hook(uint8_t* mac, int8_t rssi) {
    if (!mac) return;
    
    // We can optionally enqueue this data for MQTT publishing
    // For now, we'll just log it if in debug mode
#if (VERBOSE)
    ESP_LOGD(MQTT_TAG, "WiFi packet: MAC=%02x:%02x:%02x:%02x:%02x:%02x RSSI=%d",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], rssi);
#endif
} 