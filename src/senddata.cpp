// Basic Config
#include "senddata.h"

Ticker sendcycler;

void sendcycle() {
  xTaskNotifyFromISR(irqHandlerTask, SENDCYCLE_IRQ, eSetBits, NULL);
}

// put data to send in RTos Queues used for transmit over channels Lora and SPI
void SendPayload(uint8_t port, sendprio_t prio) {

  MessageBuffer_t
      SendBuffer; // contains MessageSize, MessagePort, MessagePrio, Message[]

  SendBuffer.MessageSize = payload.getSize();
  SendBuffer.MessagePrio = prio;

  switch (PAYLOAD_ENCODER) {
  case 1: // plain -> no mapping
  case 2: // packed -> no mapping
    SendBuffer.MessagePort = port;
    break;
  case 3: // Cayenne LPP dynamic -> all payload goes out on same port
    SendBuffer.MessagePort = CAYENNE_LPP1;
    break;
  case 4: // Cayenne LPP packed -> we need to map some paxcounter ports
    SendBuffer.MessagePort = CAYENNE_LPP2;
    switch (SendBuffer.MessagePort) {
    case COUNTERPORT:
      SendBuffer.MessagePort = CAYENNE_LPP2;
      break;
    case RCMDPORT:
      SendBuffer.MessagePort = CAYENNE_ACTUATOR;
      break;
    case TIMEPORT:
      SendBuffer.MessagePort = CAYENNE_DEVICECONFIG;
      break;
    }
    break;
  default:
    SendBuffer.MessagePort = port;
  }
  memcpy(SendBuffer.Message, payload.getBuffer(), SendBuffer.MessageSize);

// enqueue message in device's send queues
#if (HAS_LORA)
  lora_enqueuedata(&SendBuffer);
#endif
#ifdef HAS_SPI
  spi_enqueuedata(&SendBuffer);
#endif

} // SendPayload

// interrupt triggered function to prepare payload to send
void sendData() {

  uint8_t bitmask = cfg.payloadmask;
  uint8_t mask = 1;

  while (bitmask) {
    switch (bitmask & mask) {

#if ((WIFICOUNTER) || (BLECOUNTER))
    case COUNT_DATA:
      payload.reset();
      payload.addCount(macs_wifi, MAC_SNIFF_WIFI);
      if (cfg.blescan)
        payload.addCount(macs_ble, MAC_SNIFF_BLE);
      SendPayload(COUNTERPORT, prio_normal);
      // clear counter if not in cumulative counter mode
      if (cfg.countermode != 1) {
        reset_counters(); // clear macs container and reset all counters
        get_salt();       // get new salt for salting hashes
        ESP_LOGI(TAG, "Counter cleared");
      }
      break;
#endif

#if (HAS_BME)
    case MEMS_DATA:
      payload.reset();
      payload.addBME(bme_status);
      SendPayload(BMEPORT, prio_normal);
      break;
#endif

#if (HAS_GPS)
    case GPS_DATA:
      // send GPS position only if we have a fix
      if (gps.location.isValid()) {
        gpsStatus_t gps_status;
        gps_storelocation(&gps_status);
        payload.reset();
        payload.addGPS(gps_status);
        SendPayload(GPSPORT, prio_high);
      } else
        ESP_LOGD(TAG, "No valid GPS position");
      break;
#endif

#if (HAS_SENSORS)
    case SENSOR1_DATA:
      payload.reset();
      payload.addSensor(sensor_read(1));
      SendPayload(SENSOR1PORT, prio_normal);
      break;
    case SENSOR2_DATA:
      payload.reset();
      payload.addSensor(sensor_read(2));
      SendPayload(SENSOR2PORT, prio_normal);
      break;
    case SENSOR3_DATA:
      payload.reset();
      payload.addSensor(sensor_read(3));
      SendPayload(SENSOR3PORT, prio_normal);
      break;
#endif

#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
    case BATT_DATA:
      payload.reset();
      payload.addVoltage(read_voltage());
      SendPayload(BATTPORT, prio_normal);
      break;
#endif

    } // switch
    bitmask &= ~mask;
    mask <<= 1;
  } // while (bitmask)

} // sendData()

void flushQueues() {
#if (HAS_LORA)
  lora_queuereset();
#endif
#ifdef HAS_SPI
  spi_queuereset();
#endif
}