#include <ArduinoJson.h>
#include "mqtthandler.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "libpax_helpers.h"
#include "globals.h"

static const char* MQTT_TAG = "MQTT_HANDLER";

// Static task handles
static TaskHandle_t paxMqttTaskHandle = NULL;

// Current probe counts - protected by mutex
static struct {
    uint32_t pax;
    uint32_t wifi_count;
    uint32_t ble_count;
    uint32_t timestamp;
} currentCounts = {0};

static portMUX_TYPE countsMux = portMUX_INITIALIZER_UNLOCKED;

WiFiClient paxWifiClient;
PubSubClient paxMqttClient(paxWifiClient);

// FreeRTOS primitives
static QueueHandle_t mqttButtonQueue = NULL;
static portMUX_TYPE mqttMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool shouldSendMQTT = false;

// ISR handler for button press
void IRAM_ATTR buttonISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t pressTime = millis();
    
    portENTER_CRITICAL_ISR(&mqttMux);
    static uint32_t lastPressTime = 0;
    if ((pressTime - lastPressTime) > 300) {
        lastPressTime = pressTime;
        xQueueSendFromISR(mqttButtonQueue, &pressTime, &xHigherPriorityTaskWoken);
    }
    portEXIT_CRITICAL_ISR(&mqttMux);
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void paxMqttTask(void *pvParameters) {
    ESP_LOGI(MQTT_TAG, "MQTT Task started");
    
    for(;;) {
        uint32_t pressTime;
        if (xQueueReceive(mqttButtonQueue, &pressTime, pdMS_TO_TICKS(100)) == pdTRUE) {
            ESP_LOGI(MQTT_TAG, "Button press detected at %lu ms", pressTime);
            
            portENTER_CRITICAL(&mqttMux);
            shouldSendMQTT = true;
            portEXIT_CRITICAL(&mqttMux);
            
            try {
                if (shouldSendMQTT) {
                    ESP_LOGI(MQTT_TAG, "Starting data send sequence...");
                    
                    // Get current counts before stopping
                    struct count_payload_t current_count;
                    if (libpax_counter_count(&current_count) == 0) {
                        // Update our atomic counts
                        portENTER_CRITICAL(&countsMux);
                        currentCounts.pax = current_count.pax;
                        currentCounts.wifi_count = current_count.wifi_count;
                        currentCounts.ble_count = current_count.ble_count;
                        currentCounts.timestamp = millis();
                        portEXIT_CRITICAL(&countsMux);
                        
                        ESP_LOGI(MQTT_TAG, "Got current counts - PAX: %d, WiFi: %d, BLE: %d",
                                current_count.pax, current_count.wifi_count, current_count.ble_count);
                    }
                    
                    // Stop libpax counter with retry
                    ESP_LOGI(MQTT_TAG, "Stopping libpax counter...");
                    int retries = 0;
                    while (libpax_counter_stop() != 0 && retries < 3) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                        retries++;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                    
                    // Connect to WiFi and send data
                    WiFi.mode(WIFI_STA);
                    ESP_LOGI(MQTT_TAG, "Connecting to WiFi...");
                    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                    
                    int attempts = 0;
                    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                        vTaskDelay(pdMS_TO_TICKS(500));
                        ESP_LOGI(MQTT_TAG, "Attempting to connect to WiFi... (%d)", attempts + 1);
                        attempts++;
                    }
                    
                    if (WiFi.status() == WL_CONNECTED) {
                        ESP_LOGI(MQTT_TAG, "WiFi connected successfully!");
                        pax_mqtt_send_data();
                        
                        ESP_LOGI(MQTT_TAG, "Disconnecting WiFi...");
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_OFF);
                    } else {
                        ESP_LOGE(MQTT_TAG, "Failed to connect to WiFi, data not sent");
                    }
                    
                    // Restart libpax counter with retry
                    ESP_LOGI(MQTT_TAG, "Restarting libpax counter...");
                    retries = 0;
                    while (libpax_counter_start() != 0 && retries < 3) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                        retries++;
                    }
                    
                    portENTER_CRITICAL(&mqttMux);
                    shouldSendMQTT = false;
                    portEXIT_CRITICAL(&mqttMux);
                }
            } catch (const std::exception& e) {
                ESP_LOGE(MQTT_TAG, "Exception in MQTT task: %s", e.what());
                libpax_counter_start();
            } catch (...) {
                ESP_LOGE(MQTT_TAG, "Unknown exception in MQTT task");
                libpax_counter_start();
            }
        }
        taskYIELD();
    }
}

void pax_mqtt_init() {
    ESP_LOGI(MQTT_TAG, "Initializing MQTT Handler...");
    
    mqttButtonQueue = xQueueCreate(5, sizeof(uint32_t));
    if (mqttButtonQueue == NULL) {
        ESP_LOGE(MQTT_TAG, "Failed to create button queue");
        return;
    }
    
    pinMode(PAX_MQTT_TRIGGER_PIN, PAX_MQTT_TRIGGER_MODE);
    ESP_LOGI(MQTT_TAG, "Setting up trigger button on pin %d", PAX_MQTT_TRIGGER_PIN);
    
    BaseType_t xReturned = xTaskCreatePinnedToCore(
        paxMqttTask,
        "PAX_MQTT_Task",
        8192,
        NULL,
        1,
        &paxMqttTaskHandle,
        1
    );
    
    if (xReturned != pdPASS) {
        ESP_LOGE(MQTT_TAG, "Failed to create MQTT task");
        return;
    }
    
    attachInterrupt(digitalPinToInterrupt(PAX_MQTT_TRIGGER_PIN), buttonISR, FALLING);
    
    ESP_LOGI(MQTT_TAG, "MQTT Handler initialized successfully");
}

void pax_mqtt_loop() {
    vTaskDelay(pdMS_TO_TICKS(10));
}

void pax_mqtt_enqueue(uint32_t pax_count, uint32_t wifi_count, uint32_t ble_count) {
    portENTER_CRITICAL(&countsMux);
    currentCounts.pax = pax_count;
    currentCounts.wifi_count = wifi_count;
    currentCounts.ble_count = ble_count;
    currentCounts.timestamp = millis();
    portEXIT_CRITICAL(&countsMux);
    
    ESP_LOGI(MQTT_TAG, "Updated current counts: pax=%d, wifi=%d, ble=%d", 
            pax_count, wifi_count, ble_count);
}

bool pax_mqtt_connect() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(MQTT_TAG, "Cannot connect to MQTT - WiFi not connected");
        return false;
    }
    
    if (!paxMqttClient.connected()) {
        ESP_LOGI(MQTT_TAG, "Connecting to MQTT broker...");
        paxMqttClient.setServer(PAX_MQTT_SERVER, PAX_MQTT_PORT);
        paxMqttClient.setKeepAlive(PAX_MQTT_KEEPALIVE);
        
        if (paxMqttClient.connect(PAX_MQTT_CLIENTNAME)) {
            ESP_LOGI(MQTT_TAG, "Connected to MQTT broker");
            return true;
        } else {
            ESP_LOGE(MQTT_TAG, "Failed to connect to MQTT broker");
            return false;
        }
    }
    return true;
}

void pax_mqtt_send_data() {
    if (!pax_mqtt_connect()) {
        ESP_LOGE(MQTT_TAG, "Cannot send data - MQTT connection failed");
        return;
    }
    
    StaticJsonDocument<256> doc;
    JsonObject data = doc.createNestedObject("data");
    
    portENTER_CRITICAL(&countsMux);
    data["pax"] = currentCounts.pax;
    data["wifi"] = currentCounts.wifi_count;
    data["ble"] = currentCounts.ble_count;
    data["timestamp"] = currentCounts.timestamp;
    portEXIT_CRITICAL(&countsMux);
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    if (paxMqttClient.publish(PAX_MQTT_OUTTOPIC, buffer)) {
        ESP_LOGI(MQTT_TAG, "Successfully sent data to MQTT broker");
    } else {
        ESP_LOGE(MQTT_TAG, "Failed to publish data to MQTT broker");
    }
    
    paxMqttClient.disconnect();
} 