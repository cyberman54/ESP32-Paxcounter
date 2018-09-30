/* This routine is called by interrupt in regular intervals */
/* Interval can be set in paxcounter.conf (HOMECYCLE)       */

// Basic config
#include "globals.h"
#include "senddata.h"
#include "ota.h"

// Local logging tag
static const char TAG[] = "main";

// do all housekeeping
void doHousekeeping() {

  portENTER_CRITICAL(&mutexHomeCycle);
  HomeCycleIRQ = 0;
  portEXIT_CRITICAL(&mutexHomeCycle);

  // update uptime counter
  uptime();

  // check if update mode trigger switch was set
  if (cfg.runmode == 1)
    ESP.restart();

// task storage debugging //
#ifdef HAS_LORA
  ESP_LOGD(TAG, "Loraloop %d bytes left",
           uxTaskGetStackHighWaterMark(LoraTask));
#endif
  ESP_LOGD(TAG, "Wifiloop %d bytes left",
           uxTaskGetStackHighWaterMark(wifiSwitchTask));
  ESP_LOGD(TAG, "Statemachine %d bytes left",
           uxTaskGetStackHighWaterMark(stateMachineTask));
#ifdef HAS_GPS
  ESP_LOGD(TAG, "Gpsloop %d bytes left", uxTaskGetStackHighWaterMark(GpsTask));
#endif

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
    ESP_LOGI(TAG,
             "Memory full, counter cleared (heap low water mark = %d Bytes / "
             "free heap = %d bytes)",
             esp_get_minimum_free_heap_size(), ESP.getFreeHeap());
    SendData(COUNTERPORT); // send data before clearing counters
    reset_counters();      // clear macs container and reset all counters
    get_salt();            // get new salt for salting hashes

    if (esp_get_minimum_free_heap_size() <= MEM_LOW) // check again
      esp_restart(); // memory leak, reset device
  }
} // doHousekeeping()

void IRAM_ATTR homeCycleIRQ() {
  portENTER_CRITICAL(&mutexHomeCycle);
  HomeCycleIRQ++;
  portEXIT_CRITICAL(&mutexHomeCycle);
}

// uptime counter 64bit to prevent millis() rollover after 49 days
uint64_t uptime() {
  static uint32_t low32, high32;
  uint32_t new_low32 = millis();
  if (new_low32 < low32)
    high32++;
  low32 = new_low32;
  return (uint64_t)high32 << 32 | low32;
}

void reset_counters() {
  macs.clear();   // clear all macs container
  macs_total = 0; // reset all counters
  macs_wifi = 0;
  macs_ble = 0;
}

#ifndef VERBOSE
int redirect_log(const char *fmt, va_list args) {
  // do nothing
  return 0;
}
#endif
