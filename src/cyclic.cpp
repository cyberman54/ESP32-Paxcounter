/* This routine is called by interrupt in regular intervals */
/* Interval can be set in paxcounter.conf (HOMECYCLE)       */

// Basic config
#include "cyclic.h"

// Local logging tag
static const char TAG[] = "main";

uint32_t userUTCTime; // Seconds since the UTC epoch
unsigned long nextTimeSync = millis();

// do all housekeeping
void doHousekeeping() {

  // update uptime counter
  uptime();

  // check if update mode trigger switch was set
  if (cfg.runmode == 1)
    do_reset();

  spi_housekeeping();
  lora_housekeeping();

// time sync once per TIME_SYNC_INTERVAL
#ifdef TIME_SYNC_INTERVAL
  if (millis() >= nextTimeSync) {
    nextTimeSync =
        millis() + TIME_SYNC_INTERVAL * 60000; // set up next time sync period
    do_timesync();
  }
#endif

  // task storage debugging //
  ESP_LOGD(TAG, "Wifiloop %d bytes left | Taskstate = %d",
           uxTaskGetStackHighWaterMark(wifiSwitchTask),
           eTaskGetState(wifiSwitchTask));
  ESP_LOGD(TAG, "IRQhandler %d bytes left | Taskstate = %d",
           uxTaskGetStackHighWaterMark(irqHandlerTask),
           eTaskGetState(irqHandlerTask));
#ifdef HAS_GPS
  ESP_LOGD(TAG, "Gpsloop %d bytes left | Taskstate = %d",
           uxTaskGetStackHighWaterMark(GpsTask), eTaskGetState(GpsTask));
#endif
#ifdef HAS_BME
  ESP_LOGD(TAG, "Bmeloop %d bytes left | Taskstate = %d",
           uxTaskGetStackHighWaterMark(BmeTask), eTaskGetState(BmeTask));
#endif

#if (HAS_LED != NOT_A_PIN) || defined(HAS_RGB_LED)
  ESP_LOGD(TAG, "LEDloop %d bytes left | Taskstate = %d",
           uxTaskGetStackHighWaterMark(ledLoopTask),
           eTaskGetState(ledLoopTask));
#endif

// read battery voltage into global variable
#ifdef HAS_BATTERY_PROBE
  batt_voltage = read_voltage();
  ESP_LOGI(TAG, "Voltage: %dmV", batt_voltage);
#endif

// display BME sensor data
#ifdef HAS_BME
  ESP_LOGI(TAG, "BME680 Temp: %.2f°C | IAQ: %.2f | IAQacc: %d",
           bme_status.temperature, bme_status.iaq, bme_status.iaq_accuracy);
#endif

  // check free heap memory
  if (ESP.getMinFreeHeap() <= MEM_LOW) {
    ESP_LOGI(TAG,
             "Memory full, counter cleared (heap low water mark = %d Bytes / "
             "free heap = %d bytes)",
             ESP.getMinFreeHeap(), ESP.getFreeHeap());
    SendPayload(COUNTERPORT); // send data before clearing counters
    reset_counters();         // clear macs container and reset all counters
    get_salt();               // get new salt for salting hashes

    if (ESP.getMinFreeHeap() <= MEM_LOW) // check again
      do_reset();                        // memory leak, reset device
  }

// check free PSRAM memory
#ifdef BOARD_HAS_PSRAM
  if (ESP.getMinFreePsram() <= MEM_LOW) {
    ESP_LOGI(TAG, "PSRAM full, counter cleared");
    SendPayload(COUNTERPORT); // send data before clearing counters
    reset_counters();         // clear macs container and reset all counters
    get_salt();               // get new salt for salting hashes

    if (ESP.getMinFreePsram() <= MEM_LOW) // check again
      do_reset();                         // memory leak, reset device
  }
#endif

} // doHousekeeping()

// uptime counter 64bit to prevent millis() rollover after 49 days
uint64_t uptime() {
  static uint32_t low32, high32;
  uint32_t new_low32 = millis();
  if (new_low32 < low32)
    high32++;
  low32 = new_low32;
  return (uint64_t)high32 << 32 | low32;
}

uint32_t getFreeRAM() {
#ifndef BOARD_HAS_PSRAM
  return ESP.getFreeHeap();
#else
  return ESP.getFreePsram();
#endif
}

void reset_counters() {
  macs.clear();   // clear all macs container
  macs_total = 0; // reset all counters
  macs_wifi = 0;
  macs_ble = 0;
}

void do_timesync() {
#ifdef TIME_SYNC_INTERVAL

// set system time to time source GPS, if we have valid gps time
#ifdef HAS_GPS
  if (gps.time.isValid()) {
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());
// set RTC time to time source GPS, if RTC is present
#ifdef HAS_RTC
    if (!set_rtctime(RtcDateTime(now())))
      ESP_LOGE(TAG, "RTC set time failure");
#endif
    time_t tt = myTZ.toLocal(now());
    ESP_LOGI(TAG, "GPS has set system time to %02d/%02d/%d %02d:%02d:%02d",
             month(tt), day(tt), year(tt), hour(tt), minute(tt), second(tt));
    return;
  } else {
    ESP_LOGI(TAG, "No valid GPS time");
  }

  // set system time to time source LoRa Network, if network supports DevTimeReq
#elif defined LMIC_ENABLE_DeviceTimeReq
  // Schedule a network time sync request at the next possible time
  LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
  ESP_LOGI(TAG, "Network time request scheduled");
#endif // HAS_GPS

#endif // TIME_SYNC_INTERVAL
} // do_timesync()

#ifndef VERBOSE
int redirect_log(const char *fmt, va_list args) {
  // do nothing
  return 0;
}
#endif
