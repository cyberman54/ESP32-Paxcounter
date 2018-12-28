// Basic Config
#include "senddata.h"

// put data to send in RTos Queues used for transmit over channels Lora and SPI
void SendPayload(uint8_t port) {

  MessageBuffer_t SendBuffer; // contains MessageSize, MessagePort, Message[]

  SendBuffer.MessageSize = payload.getSize();
  switch (PAYLOAD_ENCODER) {
  case 1:
  case 2:
    SendBuffer.MessagePort = port;
    break;
  case 3:
    SendBuffer.MessagePort = LPP1PORT;
    break;
  case 4:
    SendBuffer.MessagePort = LPP2PORT;
    break;
  default:
    SendBuffer.MessagePort = port;
  }
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
    switch (bitmask & mask) {

    case COUNT_DATA:
      payload.reset();
      payload.addCount(macs_wifi, MAC_SNIFF_WIFI);
      if (cfg.blescan)
        payload.addCount(macs_ble, MAC_SNIFF_BLE);
      
      #ifdef HAS_GPS
      if (gps.location.isValid()) { // send GPS position only if we have a fix
        gps_read();
        payload.addGPS(gps_status);
        ESP_LOGD(TAG, "Add GPS data to counter data.");
      }else {
        ESP_LOGD(TAG, "No valid GPS position. Don't add GPS data to counter data.");
      }
      #endif
      
      SendPayload(COUNTERPORT);
      // clear counter if not in cumulative counter mode
      if (cfg.countermode != 1) {
        reset_counters(); // clear macs container and reset all counters
        get_salt();       // get new salt for salting hashes
        ESP_LOGI(TAG, "Counter cleared");
      }
      break;

#ifdef HAS_BME
    case MEMS_DATA:
      payload.reset();
      payload.addBME(bme_status);
      SendPayload(BMEPORT);
      break;
#endif

#ifdef HAS_GPS
    case GPS_DATA:
      // send GPS position only if we have a fix
      if (gps.location.isValid()) {
        gps_read();
        payload.reset();
        payload.addGPS(gps_status);
        SendPayload(GPSPORT);
      } else
        ESP_LOGD(TAG, "No valid GPS position");
      break;
#endif

#ifdef HAS_SENSORS
    case SENSOR1_DATA:
      payload.reset();
      payload.addSensor(sensor_read(1));
      SendPayload(SENSOR1PORT);
      break;
    case SENSOR2_DATA:
      payload.reset();
      payload.addSensor(sensor_read(2));
      SendPayload(SENSOR2PORT);
      break;
    case SENSOR3_DATA:
      payload.reset();
      payload.addSensor(sensor_read(3));
      SendPayload(SENSOR3PORT);
      break;
#endif

#ifdef HAS_BATTERY_PROBE
    case BATT_DATA:
      payload.reset();
      payload.addVoltage(read_voltage());
      SendPayload(BATTPORT);
      break;
#endif

    } // switch
    bitmask &= ~mask;
    mask <<= 1;
  } // while (bitmask)

} // sendCounter()

void flushQueues() {
  lora_queuereset();
  spi_queuereset();
}