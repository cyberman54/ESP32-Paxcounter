// Basic Config
#include "senddata.h"

// put data to send in RTos Queues used for transmit over channels Lora and SPI
void SendPayload(uint8_t port) {

  MessageBuffer_t SendBuffer; // contains MessageSize, MessagePort, Message[]

  SendBuffer.MessageSize = payload.getSize();
  SendBuffer.MessagePort = PAYLOAD_ENCODER <= 2
                               ? port
                               : (PAYLOAD_ENCODER == 4 ? LPP2PORT : LPP1PORT);
  memcpy(SendBuffer.Message, payload.getBuffer(), payload.getSize());

  // enqueue message in device's send queues
  lora_enqueuedata(&SendBuffer);
  spi_enqueuedata(&SendBuffer);

} // SendPayload

// interrupt triggered function to prepare payload to send
void sendCounter() {

  uint8_t bitmask = cfg.payloadmask;
  uint8_t mask = 1;

  while (bitmask) {

    payload.reset();
    switch (bitmask & mask) {

    case COUNT_DATA:
      payload.addCount(macs_wifi, cfg.blescan ? macs_ble : 0);
      SendPayload(COUNTERPORT);
      // clear counter if not in cumulative counter mode
      if (cfg.countermode != 1) {
        reset_counters(); // clear macs container and reset all counters
        get_salt();       // get new salt for salting hashes
        ESP_LOGI(TAG, "Counter cleared");
      }
      break;

    case MEMS_DATA:
#ifdef HAS_BME
      payload.addBME(bme_status);
      SendPayload(BMEPORT);
#endif
      break;

    case GPS_DATA:
#ifdef HAS_GPS
      // show NMEA data in debug mode, useful for debugging GPS
      ESP_LOGD(TAG, "GPS NMEA data: passed %d / failed: %d / with fix: %d",
               gps.passedChecksum(), gps.failedChecksum(),
               gps.sentencesWithFix());
      // send GPS position only if we have a fix
      if (gps.location.isValid()) {
        ESP_LOGD(TAG,
                 "lat=%.6f | lon=%.6f | %u Sats | HDOP=%.1f | Altitude=%um",
                 gps_status.latitude / (float)1e6,
                 gps_status.longitude / (float)1e6, gps_status.satellites,
                 gps_status.hdop / (float)100, gps_status.altitude);
        gps_read();
        payload.addGPS(gps_status);
        SendPayload(GPSPORT);
      } else
        ESP_LOGD(TAG, "No valid GPS position");
#endif
      break;

    } // switch
    bitmask &= ~mask;
    mask <<= 1;
  } // while (bitmask)

} // sendCounter()

void flushQueues() {
  lora_queuereset();
  spi_queuereset();
}