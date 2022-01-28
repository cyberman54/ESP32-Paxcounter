// Basic Config
#include "senddata.h"

void setSendIRQ(TimerHandle_t xTimer) {
  xTaskNotify(irqHandlerTask, SENDCYCLE_IRQ, eSetBits);
}

void initSendDataTimer(uint8_t sendcycle) {
  static TimerHandle_t SendDataTimer = NULL;

  if (SendDataTimer == NULL) {
    SendDataTimer =
        xTimerCreate("SendDataTimer", pdMS_TO_TICKS(sendcycle * 1000), pdTRUE,
                     (void *)0, setSendIRQ);
    xTimerStart(SendDataTimer, 0);
  } else {
    xTimerChangePeriod(SendDataTimer, pdMS_TO_TICKS(sendcycle * 1000), 0);
  }
}

// put data to send in RTos Queues used for transmit over channels Lora and SPI
void SendPayload(uint8_t port) {

  ESP_LOGD(TAG, "sending Payload for Port %d", port);

  MessageBuffer_t SendBuffer; // contains MessageSize, MessagePort, Message[]

  SendBuffer.MessageSize = payload.getSize();

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
#ifdef HAS_MQTT
  mqtt_enqueuedata(&SendBuffer);
#endif

} // SendPayload

// interrupt triggered function to prepare payload to send
void sendData() {

  uint8_t bitmask = cfg.payloadmask;
  uint8_t mask = 1;
#if (HAS_GPS)
  gpsStatus_t gps_status;
#endif
#if (HAS_SDS011)
  sdsStatus_t sds_status;
#endif

  while (bitmask) {
    switch (bitmask & mask) {

#if ((WIFICOUNTER) || (BLECOUNTER))
    case COUNT_DATA:
      payload.reset();
#if !(PAYLOAD_OPENSENSEBOX)
      ESP_LOGI(TAG, "Sending libpax wifi count: %d", libpax_macs_wifi);
      payload.addCount(libpax_macs_wifi, MAC_SNIFF_WIFI);
      if (cfg.blescan) {
        ESP_LOGI(TAG, "Sending libpax ble count: %d", libpax_macs_ble);
        payload.addCount(libpax_macs_ble, MAC_SNIFF_BLE);
      }
#endif
#if (HAS_GPS)
      if (GPSPORT == COUNTERPORT) {
        // send GPS position only if we have a fix
        if (gps_hasfix()) {
          gps_storelocation(&gps_status);
          payload.addGPS(gps_status);
        } else
          ESP_LOGD(TAG, "No valid GPS position");
      }
#endif
#if (PAYLOAD_OPENSENSEBOX)
      ESP_LOGI(TAG, "Sending libpax wifi count: %d", libpax_macs_wifi);
      payload.addCount(libpax_macs_wifi, MAC_SNIFF_WIFI);
      if (cfg.blescan) {
        ESP_LOGI(TAG, "Sending libpax ble count: %d", libpax_macs_ble);
        payload.addCount(libpax_macs_ble, MAC_SNIFF_BLE);
#endif
#if (HAS_SDS011)
        sds011_store(&sds_status);
        payload.addSDS(sds_status);
#endif
        SendPayload(COUNTERPORT);
#ifdef HAS_DISPLAY
        dp_plotCurve(libpax_macs_ble + libpax_macs_wifi, true);
#endif
#if (HAS_SDCARD)
        sdcardWriteData(libpax_macs_wifi, libpax_macs_ble
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
                        ,
                        read_voltage()
#endif
        );
#endif // HAS_SDCARD

        break; // case COUNTDATA
#endif

#if (HAS_BME)
      case MEMS_DATA:
        payload.reset();
        payload.addBME(bme_status);
        SendPayload(BMEPORT);
        break;
#endif

#if (HAS_GPS)
      case GPS_DATA:
        if (GPSPORT != COUNTERPORT) {
          // send GPS position only if we have a fix
          if (gps_hasfix()) {
            gps_storelocation(&gps_status);
            payload.reset();
            payload.addGPS(gps_status);
            SendPayload(GPSPORT);
          } else
            ESP_LOGD(TAG, "No valid GPS position");
        }
        break;
#endif

#if (HAS_SENSORS)
#if (HAS_SENSOR_1)
      case SENSOR1_DATA:
        payload.reset();
        payload.addSensor(sensor_read(1));
        SendPayload(SENSOR1PORT);
        break;
#endif
#if (HAS_SENSOR_2)
      case SENSOR2_DATA:
        payload.reset();
        payload.addSensor(sensor_read(2));
        SendPayload(SENSOR2PORT);
        break;
#endif
#if (HAS_SENSOR_3)
      case SENSOR3_DATA:
        payload.reset();
        payload.addSensor(sensor_read(3));
        SendPayload(SENSOR3PORT);
        break;
#endif
#endif

#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
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
  }   // sendData()

  void flushQueues(void) {
    rcmd_queuereset();
#if (HAS_LORA)
    lora_queuereset();
#endif
#ifdef HAS_SPI
    spi_queuereset();
#endif
#ifdef HAS_MQTT
    mqtt_queuereset();
#endif
  }

  bool allQueuesEmtpy(void) {
    uint32_t rc = rcmd_queuewaiting();
#if (HAS_LORA)
    rc += lora_queuewaiting();
#endif
#ifdef HAS_SPI
    rc += spi_queuewaiting();
#endif
#ifdef HAS_MQTT
    rc += mqtt_queuewaiting();
#endif
    return (rc == 0) ? true : false;
  }
