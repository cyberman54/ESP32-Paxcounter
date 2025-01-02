#include <ArduinoJson.h>
#include "mqtthandler.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "libpax_helpers.h"
#include "globals.h"
#include <time.h>

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

// Device detection buffer - protected by mutex
static struct {
    DeviceData devices[50];  // Buffer for device detections
    size_t count;
} deviceBuffer = {0};

static portMUX_TYPE countsMux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE deviceMux = portMUX_INITIALIZER_UNLOCKED;

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
                        
                        // Send both count data and device detections
                        pax_mqtt_send_data();
                        pax_mqtt_send_devices();
                        
                        ESP_LOGI(MQTT_TAG, "Disconnecting WiFi...");
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_OFF);
                    } else {
                        ESP_LOGE(MQTT_TAG, "Failed to connect to WiFi, data not sent");
                    }
                    
                    // Clear device buffer after sending
                    portENTER_CRITICAL(&deviceMux);
                    deviceBuffer.count = 0;
                    portEXIT_CRITICAL(&deviceMux);
                    
                    portENTER_CRITICAL(&mqttMux);
                    shouldSendMQTT = false;
                    portEXIT_CRITICAL(&mqttMux);
                }
            } catch (const std::exception& e) {
                ESP_LOGE(MQTT_TAG, "Exception in MQTT task: %s", e.what());
            } catch (...) {
                ESP_LOGE(MQTT_TAG, "Unknown exception in MQTT task");
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

void pax_mqtt_enqueue_device(const uint8_t* mac, int8_t rssi, bool is_wifi) {
    if (!mac) {
        ESP_LOGE(MQTT_TAG, "Invalid MAC address pointer");
        return;
    }

    // Create temporary device data outside critical section
    DeviceData tempDevice;
    memcpy(tempDevice.mac, mac, 6);
    tempDevice.rssi = rssi;
    tempDevice.is_wifi = is_wifi;
    tempDevice.timestamp = millis();

    // Minimize time in critical section
    portENTER_CRITICAL(&deviceMux);
    if (deviceBuffer.count < 50) {
        deviceBuffer.devices[deviceBuffer.count] = tempDevice;
        deviceBuffer.count++;
        portEXIT_CRITICAL(&deviceMux);
        
        // ESP_LOGI(MQTT_TAG, "Enqueued device: MAC=%02x:%02x:%02x:%02x:%02x:%02x RSSI=%d Type=%s", 
        //         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        //         rssi, is_wifi ? "WiFi" : "BLE");
    } else {
        portEXIT_CRITICAL(&deviceMux);
        // ESP_LOGW(MQTT_TAG, "Device buffer full, dropping packet");
    }
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
    // Use current time instead of millis
    time_t now;
    time(&now);
    data["timestamp"] = now;
    portEXIT_CRITICAL(&countsMux);
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    if (paxMqttClient.publish(PAX_MQTT_OUTTOPIC, buffer)) {
        ESP_LOGI(MQTT_TAG, "Successfully sent count data to MQTT broker");
    } else {
        ESP_LOGE(MQTT_TAG, "Failed to publish count data to MQTT broker");
    }
}

void pax_mqtt_send_devices() {
    if (!pax_mqtt_connect()) {
        return;
    }
    
    // Copy data we want to send while in critical section
    DeviceData devices_to_send[50];
    size_t count;
    
    portENTER_CRITICAL(&deviceMux);
    count = deviceBuffer.count;
    memcpy(devices_to_send, deviceBuffer.devices, count * sizeof(DeviceData));
    deviceBuffer.count = 0;  // Clear buffer after copying
    portEXIT_CRITICAL(&deviceMux);
    
    // Now send each device outside of critical section
    for (size_t i = 0; i < count; i++) {
        StaticJsonDocument<256> doc;
        JsonObject device = doc.createNestedObject("device");
        
        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                devices_to_send[i].mac[0], devices_to_send[i].mac[1],
                devices_to_send[i].mac[2], devices_to_send[i].mac[3],
                devices_to_send[i].mac[4], devices_to_send[i].mac[5]);
        
        device["mac"] = mac;
        device["rssi"] = devices_to_send[i].rssi;
        device["type"] = devices_to_send[i].is_wifi ? "wifi" : "ble";
        // Use current time instead of millis
        time_t now;
        time(&now);
        device["timestamp"] = now;
        
        char buffer[256];
        serializeJson(doc, buffer);
        
        // Allow other tasks to run between publishes
        vTaskDelay(pdMS_TO_TICKS(10));
        
        if (paxMqttClient.publish(PAX_MQTT_DEVICE_TOPIC, buffer)) {
            ESP_LOGI(MQTT_TAG, "Successfully sent device data to MQTT broker");
        } else {
            ESP_LOGE(MQTT_TAG, "Failed to publish device data to MQTT broker");
            // Try to reconnect if we lost connection
            if (!pax_mqtt_connect()) {
                ESP_LOGE(MQTT_TAG, "Lost MQTT connection and failed to reconnect");
                break;
            }
        }
    }
    
    paxMqttClient.disconnect();
}

// Hook function implementation for WiFi sniffer
void IRAM_ATTR wifi_packet_handler_hook(uint8_t* mac, int8_t rssi) {
    if (!mac) return;
    
    // Minimize logging in ISR context
    pax_mqtt_enqueue_device(mac, rssi, true);
} 