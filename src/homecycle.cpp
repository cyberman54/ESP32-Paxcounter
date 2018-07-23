/* This routine is called by interrupt in regular intervals */
/* Interval can be set in paxcounter.conf (HOMECYCLE)       */

// Basic config
#include "globals.h"
#include "senddata.h"

// Local logging tag
static const char TAG[] = "main";

// do all housekeeping
void doHomework() {
// read battery voltage into global variable
#ifdef HAS_BATTERY_PROBE
  batt_voltage = read_voltage();
  ESP_LOGI(TAG, "Measured Voltage: %dmV", batt_voltage);
#endif

// sync time & date if we have valid gps time
#ifdef HAS_GPS
  if (gps.time.isValid()) {
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());
    ESP_LOGI(TAG, "Time synced to %02d:%02d:%02d", hour(), minute(), second());
  } else {
    ESP_LOGI(TAG, "No valid GPS time");
  }
#endif

  // check free memory
  if (esp_get_minimum_free_heap_size() <= MEM_LOW) {
    ESP_LOGW(TAG,
             "Memory full, counter cleared (heap low water mark = %d Bytes / "
             "free heap = %d bytes)",
             esp_get_minimum_free_heap_size(), ESP.getFreeHeap());
    senddata(COUNTERPORT); // send data before clearing counters
    reset_counters();      // clear macs container and reset all counters
    reset_salt();          // get new salt for salting hashes
  }
}

void checkHousekeeping() {
  if (HomeCycleIRQ) {
    portENTER_CRITICAL(&timerMux);
    HomeCycleIRQ = 0;
    portEXIT_CRITICAL(&timerMux);
    doHomework();
  }
}

void IRAM_ATTR homeCycleIRQ() {
  portENTER_CRITICAL(&timerMux);
  HomeCycleIRQ++;
  portEXIT_CRITICAL(&timerMux);
}