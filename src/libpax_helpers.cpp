#include "libpax_helpers.h"
#include "mqtthandler.h"

static const char* PAX_TAG = "PAX_HELPER";

// libpax payload
struct count_payload_t count_from_libpax;

// Callback for libpax counter updates
void pax_counter_callback() {
    // Get current counts before they change
    struct count_payload_t current_count;
    if (libpax_counter_count(&current_count) == 0) {
        ESP_LOGI(PAX_TAG, "Counter callback triggered with counts - PAX: %d, WiFi: %d, BLE: %d", 
                 current_count.pax, current_count.wifi_count, current_count.ble_count);
        
        // Update MQTT counts
        pax_mqtt_enqueue(current_count.pax, current_count.wifi_count, current_count.ble_count);
        
        // Call the original callback for system functionality
        setSendIRQ();
    }
}

void init_libpax(void) {
    ESP_LOGI(PAX_TAG, "Initializing libpax with callback...");
    
    // Initialize with our callback
    int result = libpax_counter_init(pax_counter_callback, &count_from_libpax, cfg.sendcycle * 2,
                      cfg.countermode);
                      
    if (result == 0) {
        ESP_LOGI(PAX_TAG, "Starting libpax counter...");
        result = libpax_counter_start();
        if (result == 0) {
            ESP_LOGI(PAX_TAG, "Libpax initialization complete");
        } else {
            ESP_LOGE(PAX_TAG, "Failed to start libpax counter: %d", result);
        }
    } else {
        ESP_LOGE(PAX_TAG, "Failed to initialize libpax: %d", result);
    }
}