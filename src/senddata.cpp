// Basic Config
#include "globals.h"

void senddata(uint8_t port) {

  // Prepare payload with counter and, if applicable, gps data
  payload.reset();
  payload.addCount(macs_wifi, cfg.blescan ? macs_ble : 0);

  // append GPS data, if present
#ifdef HAS_GPS
  if ((cfg.gpsmode) && (gps.location.isValid())) {
    gps_read();
    payload.addGPS(gps_status);
  }
#endif

#ifdef HAS_LORA
  // Check if there is a pending TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    ESP_LOGI(TAG, "LoRa busy, data not sent");
    sprintf(display_line7, "LORA BUSY");
  } else {
    // send payload via LoRa
    LMIC_setTxData2(PAYLOADPORT, payload.getBuffer(), payload.getSize(),
                    (cfg.countermode & 0x02));
    ESP_LOGI(TAG, "%d bytes queued to send on LoRa", payload.getSize());
    sprintf(display_line7, "PACKET QUEUED");
  }
#endif

#ifdef HAS_SPI
  // code for sending payload via SPI to come
  ESP_LOGI(TAG, "%d bytes sent on SPI", payload.getSize());
#endif

  // clear counter if not in cumulative counter mode
  if (cfg.countermode != 1) {
    reset_counters(); // clear macs container and reset all counters
    reset_salt();     // get new salt for salting hashes
    ESP_LOGI(TAG, "Counter cleared (countermode = %d)", cfg.countermode);
  }

} // senddata