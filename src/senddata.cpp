// Basic Config
#include "globals.h"

void senddata(uint8_t port) {

#ifdef HAS_LORA
  // Check if there is a pending TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    ESP_LOGI(TAG, "LoRa busy, data not sent");
    sprintf(display_line7, "LORA BUSY");
  } else {
    LMIC_setTxData2(PAYLOAD_ENCODER == 4 ? CAYENNEPORT : port,
                    payload.getBuffer(), payload.getSize(),
                    (cfg.countermode & 0x02));

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