// Basic Config
#include "globals.h"

void senddata(uint8_t port) {

#ifdef HAS_LORA
  // Check if there is a pending TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    ESP_LOGI(TAG, "LoRa busy, data not sent");
    sprintf(display_line7, "LORA BUSY");
  } else {
    LMIC_setTxData2(
        PAYLOAD_ENCODER <= 2 ? port
                             : (PAYLOAD_ENCODER == 4 ? LPP2PORT : LPP1PORT),
        payload.getBuffer(), payload.getSize(), (cfg.countermode & 0x02));

    ESP_LOGI(TAG, "%d bytes queued to send on LoRa", payload.getSize());
    sprintf(display_line7, "PACKET QUEUED");
  }
#endif

#ifdef HAS_SPI
  // to come here: code for sending payload to a local master via SPI
  ESP_LOGI(TAG, "%d bytes sent on SPI", payload.getSize());
#endif

  // clear counter if not in cumulative counter mode
  if ((port == COUNTERPORT) && (cfg.countermode != 1)) {
    reset_counters(); // clear macs container and reset all counters
    reset_salt();     // get new salt for salting hashes
    ESP_LOGI(TAG, "Counter cleared");
  }

} // senddata

void sendPayload() {

  if (SendCycleTimerIRQ) {
    portENTER_CRITICAL(&timerMux);
    SendCycleTimerIRQ = 0;
    portEXIT_CRITICAL(&timerMux);

    // append counter data to payload
    payload.reset();
    payload.addCount(macs_wifi, cfg.blescan ? macs_ble : 0);
    // append GPS data, if present

#ifdef HAS_GPS
    // show NMEA data in debug mode, useful for debugging GPS on board
    // connection
    ESP_LOGD(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
             gps.passedChecksum(), gps.failedChecksum(),
             gps.sentencesWithFix());
    // log GPS position if we have a fix and gps data mode is enabled
    if ((cfg.gpsmode) && (gps.location.isValid())) {
      gps_read();
      payload.addGPS(gps_status);
      ESP_LOGD(TAG, "lat=%.6f | lon=%.6f | %u Sats | HDOP=%.1f | Altitude=%um",
               gps_status.latitude / (float)1e6,
               gps_status.longitude / (float)1e6, gps_status.satellites,
               gps_status.hdop / (float)100, gps_status.altitude);
    } else {
      ESP_LOGD(TAG, "No valid GPS position or GPS data mode disabled");
    }
#endif
    senddata(COUNTERPORT);
  }
} // sendpayload();