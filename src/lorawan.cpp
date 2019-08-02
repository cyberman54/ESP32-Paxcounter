// Basic Config
#if (HAS_LORA)
#include "lorawan.h"
#endif

// Local logging Tag
static const char TAG[] = "lora";

#if (HAS_LORA)

#if CLOCK_ERROR_PROCENTAGE > 7
#warning CLOCK_ERROR_PROCENTAGE value in lmic_config.h is too high; values > 7 will cause side effects
#endif

#if (TIME_SYNC_LORAWAN)
#ifndef LMIC_ENABLE_DeviceTimeReq
#define LMIC_ENABLE_DeviceTimeReq 1
#endif
#endif

osjob_t sendjob;
QueueHandle_t LoraSendQueue;
TaskHandle_t lmicTask = NULL;

class MyHalConfig_t : public Arduino_LMIC::HalConfiguration_t {

public:
  MyHalConfig_t(){};
  virtual void begin(void) override {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  }
};

MyHalConfig_t myHalConfig{};

// LMIC pin mapping

const lmic_pinmap lmic_pins = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_RST,
    .dio = {LORA_IRQ, LORA_IO1,
            LORA_IO2 == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_IO2},
    // optional: set polarity of rxtx pin.
    .rxtx_rx_active = 0,
    // optional: set RSSI cal for listen-before-talk
    // this value is in dB, and is added to RSSI
    // measured prior to decision.
    // Must include noise guardband! Ignored in US,
    // EU, IN, other markets where LBT is not required.
    .rssi_cal = 0,
    // optional: override LMIC_SPI_FREQ if non-zero
    .spi_freq = 0,
    .pConfig = &myHalConfig};

// DevEUI generator using devices's MAC address
void gen_lora_deveui(uint8_t *pdeveui) {
  uint8_t *p = pdeveui, dmac[6];
  int i = 0;
  esp_efuse_mac_get_default(dmac);
  // deveui is LSB, we reverse it so TTN DEVEUI display
  // will remain the same as MAC address
  // MAC is 6 bytes, devEUI 8, set first 2 ones
  // with an arbitrary value
  *p++ = 0xFF;
  *p++ = 0xFE;
  // Then next 6 bytes are mac address reversed
  for (i = 0; i < 6; i++) {
    *p++ = dmac[5 - i];
  }
}

/* new version, does it with well formed mac according IEEE spec, but is
breaking change
// DevEUI generator using devices's MAC address
void gen_lora_deveui(uint8_t *pdeveui) {
  uint8_t *p = pdeveui, dmac[6];
  ESP_ERROR_CHECK(esp_efuse_mac_get_default(dmac));
  // deveui is LSB, we reverse it so TTN DEVEUI display
  // will remain the same as MAC address
  // MAC is 6 bytes, devEUI 8, set middle 2 ones
  // to an arbitrary value
  *p++ = dmac[5];
  *p++ = dmac[4];
  *p++ = dmac[3];
  *p++ = 0xfe;
  *p++ = 0xff;
  *p++ = dmac[2];
  *p++ = dmac[1];
  *p++ = dmac[0];
}
*/

// Function to do a byte swap in a byte array
void RevBytes(unsigned char *b, size_t c) {
  u1_t i;
  for (i = 0; i < c / 2; i++) {
    unsigned char t = b[i];
    b[i] = b[c - 1 - i];
    b[c - 1 - i] = t;
  }
}

// LMIC callback functions
void os_getDevKey(u1_t *buf) { memcpy(buf, APPKEY, 16); }

void os_getArtEui(u1_t *buf) {
  memcpy(buf, APPEUI, 8);
  RevBytes(buf, 8); // TTN requires it in LSB First order, so we swap bytes
}

void os_getDevEui(u1_t *buf) {
  int i = 0, k = 0;
  memcpy(buf, DEVEUI, 8); // get fixed DEVEUI from loraconf.h
  for (i = 0; i < 8; i++) {
    k += buf[i];
  }
  if (k) {
    RevBytes(buf, 8); // use fixed DEVEUI and swap bytes to LSB format
  } else {
    gen_lora_deveui(buf); // generate DEVEUI from device's MAC
  }

// Get MCP 24AA02E64 hardware DEVEUI (override default settings if found)
#ifdef MCP_24AA02E64_I2C_ADDRESS
  get_hard_deveui(buf);
  RevBytes(buf, 8); // swap bytes to LSB format
#endif
}

void get_hard_deveui(uint8_t *pdeveui) {
  // read DEVEUI from Microchip 24AA02E64 2Kb serial eeprom if present
#ifdef MCP_24AA02E64_I2C_ADDRESS

  uint8_t i2c_ret;

  // Init this just in case, no more to 100KHz
  Wire.begin(MY_OLED_SDA, MY_OLED_SCL, 100000);
  Wire.beginTransmission(MCP_24AA02E64_I2C_ADDRESS);
  Wire.write(MCP_24AA02E64_MAC_ADDRESS);
  i2c_ret = Wire.endTransmission();

  // check if device was seen on i2c bus
  if (i2c_ret == 0) {
    char deveui[32] = "";
    uint8_t data;

    Wire.beginTransmission(MCP_24AA02E64_I2C_ADDRESS);
    Wire.write(MCP_24AA02E64_MAC_ADDRESS);
    Wire.endTransmission();

    Wire.requestFrom(MCP_24AA02E64_I2C_ADDRESS, 8);
    while (Wire.available()) {
      data = Wire.read();
      sprintf(deveui + strlen(deveui), "%02X ", data);
      *pdeveui++ = data;
    }
    ESP_LOGI(TAG, "Serial EEPROM found, read DEVEUI %s", deveui);
  } else
    ESP_LOGI(TAG, "Could not read DEVEUI from serial EEPROM");

  // Set back to 400KHz to speed up OLED
  Wire.setClock(400000);
#endif // MCP 24AA02E64
}

#if (VERBOSE)

// Display OTAA keys
void showLoraKeys(void) {
  // LMIC may not have used callback to fill
  // all EUI buffer so we do it here to a temp
  // buffer to be able to display them
  uint8_t buf[32];
  os_getDevEui((u1_t *)buf);
  printKey("DevEUI", buf, 8, true);
  os_getArtEui((u1_t *)buf);
  printKey("AppEUI", buf, 8, true);
  os_getDevKey((u1_t *)buf);
  printKey("AppKey", buf, 16, false);
}

#endif // VERBOSE

void onEvent(ev_t ev) {
  char buff[24] = "";

  switch (ev) {

  case EV_SCAN_TIMEOUT:
    strcpy_P(buff, PSTR("SCAN TIMEOUT"));
    break;

  case EV_BEACON_FOUND:
    strcpy_P(buff, PSTR("BEACON FOUND"));
    break;

  case EV_BEACON_MISSED:
    strcpy_P(buff, PSTR("BEACON MISSED"));
    break;

  case EV_BEACON_TRACKED:
    strcpy_P(buff, PSTR("BEACON TRACKED"));
    break;

  case EV_JOINING:
    strcpy_P(buff, PSTR("JOINING"));
    break;

  case EV_JOINED:
    strcpy_P(buff, PSTR("JOINED"));
    sprintf(display_line6, " "); // clear previous lmic status
    // set data rate adaptation according to saved setting
    LMIC_setAdrMode(cfg.adrmode);
    // set cyclic lmic link check to off if no ADR because is not supported by
    // ttn (but enabled by lmic after join)
    LMIC_setLinkCheckMode(cfg.adrmode);
    // Set data rate and transmit power (note: txpower seems to be ignored by
    // the library)
    switch_lora(cfg.lorasf, cfg.txpower);
    // kickoff first send job
    os_setCallback(&sendjob, lora_send);
    // show effective LoRa parameters after join
    ESP_LOGI(TAG, "DEVaddr=%08X", LMIC.devaddr);
    break;

  case EV_RFU1:
    strcpy_P(buff, PSTR("RFU1"));
    break;

  case EV_JOIN_FAILED:
    strcpy_P(buff, PSTR("JOIN FAILED"));
    break;

  case EV_REJOIN_FAILED:
    strcpy_P(buff, PSTR("REJOIN FAILED"));
    break;

  case EV_TXCOMPLETE:

#if (TIME_SYNC_LORASERVER)
    // if last packet sent was a timesync request, store TX timestamp
    if (LMIC.pendTxPort == TIMEPORT)
      store_time_sync_req(osticks2ms(LMIC.txend)); // milliseconds
#endif

    strcpy_P(buff, (LMIC.txrxFlags & TXRX_ACK) ? PSTR("RECEIVED ACK")
                                               : PSTR("TX COMPLETE"));
    sprintf(display_line6, " "); // clear previous lmic status

    if (LMIC.dataLen) { // did we receive payload data -> display info
      ESP_LOGI(TAG, "Received %d bytes of payload, RSSI %d SNR %d",
               LMIC.dataLen, LMIC.rssi, LMIC.snr / 4);
      sprintf(display_line6, "RSSI %d SNR %d", LMIC.rssi, LMIC.snr / 4);

      if (LMIC.txrxFlags & TXRX_PORT) { // FPort -> use to switch

        switch (LMIC.frame[LMIC.dataBeg - 1]) {

        case RCMDPORT: // opcode -> call rcommand interpreter
          rcommand(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
          break;

        default:

#if (TIME_SYNC_LORASERVER)
          // timesync answer -> call timesync processor
          if ((LMIC.frame[LMIC.dataBeg - 1] >= TIMEANSWERPORT_MIN) &&
              (LMIC.frame[LMIC.dataBeg - 1] <= TIMEANSWERPORT_MAX)) {
            recv_timesync_ans(LMIC.frame[LMIC.dataBeg - 1],
                              LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
            break;
          }
#endif
          // unknown port -> display info
          ESP_LOGI(TAG, "Received data on unsupported port #%d",
                   LMIC.frame[LMIC.dataBeg - 1]);
          break;
        }
      }
    }
    break;

  case EV_LOST_TSYNC:
    strcpy_P(buff, PSTR("LOST TSYNC"));
    break;

  case EV_RESET:
    strcpy_P(buff, PSTR("RESET"));
    break;

  case EV_RXCOMPLETE:
    // data received in ping slot
    strcpy_P(buff, PSTR("RX COMPLETE"));
    break;

  case EV_LINK_DEAD:
    strcpy_P(buff, PSTR("LINK DEAD"));
    break;

  case EV_LINK_ALIVE:
    strcpy_P(buff, PSTR("LINK_ALIVE"));
    break;

  case EV_SCAN_FOUND:
    strcpy_P(buff, PSTR("SCAN FOUND"));
    break;

  case EV_TXSTART:
    if (!(LMIC.opmode & OP_JOINING)) {
#if (TIME_SYNC_LORASERVER)
      // if last packet sent was a timesync request, store TX time
      if (LMIC.pendTxPort == TIMEPORT)
        strcpy_P(buff, PSTR("TX TIMESYNC"));
      else
#endif
        strcpy_P(buff, PSTR("TX START"));
    }
    break;

  case EV_TXCANCELED:
    strcpy_P(buff, PSTR("TX CANCELLED"));
    break;

  case EV_RXSTART:
    strcpy_P(buff, PSTR("RX START"));
    break;

  case EV_JOIN_TXCOMPLETE:
    strcpy_P(buff, PSTR("JOIN WAIT"));
    break;

  default:
    sprintf_P(buff, PSTR("LMIC EV %d"), ev);
    break;
  }

  // Log & Display if asked
  if (*buff) {
    ESP_LOGI(TAG, "%s", buff);
    sprintf(display_line7, buff);
  }
}

// helper function to assign LoRa datarates to numeric spreadfactor values
void switch_lora(uint8_t sf, uint8_t tx) {
  if (tx > 20)
    return;
  cfg.txpower = tx;
  switch (sf) {
  case 7:
    LMIC_setDrTxpow(DR_SF7, tx);
    cfg.lorasf = sf;
    break;
  case 8:
    LMIC_setDrTxpow(DR_SF8, tx);
    cfg.lorasf = sf;
    break;
  case 9:
    LMIC_setDrTxpow(DR_SF9, tx);
    cfg.lorasf = sf;
    break;
  case 10:
    LMIC_setDrTxpow(DR_SF10, tx);
    cfg.lorasf = sf;
    break;
  case 11:
#if defined(CFG_us915)
    LMIC_setDrTxpow(DR_SF11CR, tx);
    cfg.lorasf = sf;
    break;
#else
    LMIC_setDrTxpow(DR_SF11, tx);
    cfg.lorasf = sf;
    break;
#endif
  case 12:
#if defined(CFG_us915)
    LMIC_setDrTxpow(DR_SF12CR, tx);
    cfg.lorasf = sf;
    break;
#else
    LMIC_setDrTxpow(DR_SF12, tx);
    cfg.lorasf = sf;
    break;
#endif
  default:
    break;
  }
}

void lora_send(osjob_t *job) {
  MessageBuffer_t SendBuffer;
  // Check if there is a pending TX/RX job running, if yes don't eat data
  // since it cannot be sent right now
  if ((LMIC.opmode & (OP_JOINING | OP_REJOIN | OP_TXDATA | OP_POLL)) != 0) {
    // waiting for LoRa getting ready
  } else {
    if (xQueueReceive(LoraSendQueue, &SendBuffer, (TickType_t)0) == pdTRUE) {
      // SendBuffer now filled with next payload from queue
      if (!LMIC_setTxData2(SendBuffer.MessagePort, SendBuffer.Message,
                           SendBuffer.MessageSize, (cfg.countermode & 0x02))) {
        ESP_LOGI(TAG, "%d byte(s) sent to LoRa", SendBuffer.MessageSize);
      } else {
        ESP_LOGE(TAG, "could not send %d byte(s) to LoRa",
                 SendBuffer.MessageSize);
      }
      // sprintf(display_line7, "PACKET QUEUED");
    }
  }
  // reschedule job every 0,5 - 1 sec. including a bit of random to prevent
  // systematic collisions
  os_setTimedCallback(job, os_getTime() + 500 + ms2osticks(random(500)),
                      lora_send);
}

esp_err_t lora_stack_init() {
  assert(SEND_QUEUE_SIZE);
  LoraSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (LoraSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create LORA send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "LORA send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * sizeof(MessageBuffer_t));

  // starting lorawan stack
  ESP_LOGI(TAG, "Starting LMIC...");
  xTaskCreatePinnedToCore(lmictask,   // task function
                          "lmictask", // name of task
                          4096,       // stack size of task
                          (void *)1,  // parameter of the task
                          2,          // priority of the task
                          &lmicTask,  // task handle
                          1);         // CPU core

  if (!LMIC_startJoining()) { // start joining
    ESP_LOGI(TAG, "Already joined");
  }

  return ESP_OK; // continue main program
}

void lora_enqueuedata(MessageBuffer_t *message, sendprio_t prio) {
  // enqueue message in LORA send queue
  BaseType_t ret;
  MessageBuffer_t DummyBuffer;
  switch (prio) {
  case prio_high:
    // clear space in queue if full, then fallthrough to normal
    if (uxQueueSpacesAvailable(LoraSendQueue) == 0)
      xQueueReceive(LoraSendQueue, &DummyBuffer, (TickType_t)0);
  case prio_normal:
    ret = xQueueSendToFront(LoraSendQueue, (void *)message, (TickType_t)0);
    break;
  case prio_low:
  default:
    ret = xQueueSendToBack(LoraSendQueue, (void *)message, (TickType_t)0);
    break;
  }
  if (ret != pdTRUE)
    ESP_LOGW(TAG, "LORA sendqueue is full");
}

void lora_queuereset(void) { xQueueReset(LoraSendQueue); }

void lora_housekeeping(void) {
  // ESP_LOGD(TAG, "loraloop %d bytes left",
  // uxTaskGetStackHighWaterMark(LoraTask));
}

#if (TIME_SYNC_LORAWAN)
void IRAM_ATTR user_request_network_time_callback(void *pVoidUserUTCTime,
                                                  int flagSuccess) {
  // Explicit conversion from void* to uint32_t* to avoid compiler errors
  time_t *pUserUTCTime = (time_t *)pVoidUserUTCTime;

  // A struct that will be populated by LMIC_getNetworkTimeReference.
  // It contains the following fields:
  //  - tLocal: the value returned by os_GetTime() when the time
  //            request was sent to the gateway, and
  //  - tNetwork: the seconds between the GPS epoch and the time
  //              the gateway received the time request
  lmic_time_reference_t lmicTimeReference;

  if (flagSuccess != 1) {
    ESP_LOGW(TAG, "LoRaWAN network did not answer time request");
    return;
  }

  // Populate lmic_time_reference
  flagSuccess = LMIC_getNetworkTimeReference(&lmicTimeReference);
  if (flagSuccess != 1) {
    ESP_LOGW(TAG, "LoRaWAN time request failed");
    return;
  }

  // mask application irq to ensure accurate timing
  mask_user_IRQ();

  // Update userUTCTime, considering the difference between the GPS and UTC
  // time, and the leap seconds until year 2019
  *pUserUTCTime = lmicTimeReference.tNetwork + 315964800;
  // Current time, in ticks
  ostime_t ticksNow = os_getTime();
  // Time when the request was sent, in ticks
  ostime_t ticksRequestSent = lmicTimeReference.tLocal;
  // Add the delay between the instant the time was transmitted and
  // the current time
  time_t requestDelaySec = osticks2ms(ticksNow - ticksRequestSent) / 1000;

  // Update system time with time read from the network
  setMyTime(*pUserUTCTime + requestDelaySec, 0);

finish:
  // end of time critical section: release app irq lock
  unmask_user_IRQ();

} // user_request_network_time_callback
#endif // TIME_SYNC_LORAWAN

// LMIC lorawan stack task
void lmictask(void *pvParameters) {
  configASSERT(((uint32_t)pvParameters) == 1); // FreeRTOS check

  os_init();    // initialize lmic run-time environment on core 1
  LMIC_reset(); // initialize lmic MAC
  LMIC_setLinkCheckMode(0);
  // This tells LMIC to make the receive windows bigger, in case your clock is
  // faster or slower. This causes the transceiver to be earlier switched on,
  // so consuming more power. You may sharpen (reduce) CLOCK_ERROR_PERCENTAGE
  // in src/lmic_config.h if you are limited on battery.
  LMIC_setClockError(MAX_CLOCK_ERROR * CLOCK_ERROR_PROCENTAGE / 100);
  // Set the data rate to Spreading Factor 7.  This is the fastest supported
  // rate for 125 kHz channels, and it minimizes air time and battery power.
  // Set the transmission power to 14 dBi (25 mW).
  LMIC_setDrTxpow(DR_SF7, 14);

#if defined(CFG_US915) || defined(CFG_au921)
  // in the US, with TTN, it saves join time if we start on subband 1
  // (channels 8-15). This will get overridden after the join by parameters
  // from the network. If working with other networks or in other regions,
  // this will need to be changed.
  LMIC_selectSubBand(1);
#endif

  while (1) {
    os_runloop_once(); // execute lmic scheduled jobs and events
    delay(2);          // yield to CPU
  }
} // lmictask

#endif // HAS_LORA
