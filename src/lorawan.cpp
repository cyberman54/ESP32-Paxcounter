// Basic Config

#if (HAS_LORA)
#include "lorawan.h"

// Local logging Tag
static const char TAG[] = __FILE__;

#if CLOCK_ERROR_PROCENTAGE > 7
#warning CLOCK_ERROR_PROCENTAGE value in lmic_config.h is too high; values > 7 will cause side effects
#endif

#if (TIME_SYNC_LORAWAN)
#ifndef LMIC_ENABLE_DeviceTimeReq
#define LMIC_ENABLE_DeviceTimeReq 1
#endif
#endif

static QueueHandle_t LoraSendQueue;
TaskHandle_t lmicTask = NULL, lorasendTask = NULL;
char lmic_event_msg[LMIC_EVENTMSG_LEN]; // display buffer for LMIC event message

class MyHalConfig_t : public Arduino_LMIC::HalConfiguration_t {
public:
  MyHalConfig_t(){};

  // set SPI pins to board configuration, pins may come from pins_arduino.h
  void begin(void) override {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  }

  // void end(void) override

  // ostime_t setModuleActive(bool state) override
};

static MyHalConfig_t myHalConfig{};

// LMIC pin mapping for Hope RFM95 / HPDtek HPD13A transceivers
static const lmic_pinmap myPinmap = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_RST,
    .dio = {LORA_IRQ, LORA_IO1,
            LORA_IO2 == NOT_A_PIN ? LMIC_UNUSED_PIN : LORA_IO2},
    .rxtx_rx_active = LMIC_UNUSED_PIN,
    .rssi_cal = 10,
    .spi_freq = 8000000, // 8MHz
    .pConfig = &myHalConfig};

void lora_setupForNetwork(bool preJoin) {
  if (preJoin) {
#if CFG_LMIC_US_like
    // in the US, with TTN, it saves join time if we start on subband 1
    // (channels 8-15). This will get overridden after the join by
    // parameters from the network. If working with other networks or in
    // other regions, this will need to be changed.
    LMIC_selectSubBand(1);
#elif CFG_LMIC_EU_like
    // settings for TheThingsNetwork
    // Enable link check validation
    LMIC_setLinkCheckMode(1);
#endif

  } else {
    // set data rate adaptation according to saved setting
    LMIC_setAdrMode(cfg.adrmode);
    // set data rate and transmit power to stored device values if no ADR
    if (!cfg.adrmode)
      LMIC_setDrTxpow(assertDR(cfg.loradr), cfg.txpower);
    // show current devaddr
    ESP_LOGI(TAG, "DEVaddr: 0x%08X | Network ID: 0x%06X | Network Type: %d",
             LMIC.devaddr, LMIC.netid & 0x001FFFFF, LMIC.netid & 0x00E00000);
    ESP_LOGI(TAG, "RSSI: %d | SNR: %d", LMIC.rssi, (LMIC.snr + 2) / 4);
    ESP_LOGI(TAG, "Radio parameters: %s | %s | %s",
             getSfName(updr2rps(LMIC.datarate)),
             getBwName(updr2rps(LMIC.datarate)),
             getCrName(updr2rps(LMIC.datarate)));
  }
}

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
  esp_efuse_mac_get_default(dmac);
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
void os_getDevKey(u1_t *buf) {
#ifndef LORA_ABP
  memcpy(buf, APPKEY, 16);
#endif
}

void os_getArtEui(u1_t *buf) {
#ifndef LORA_ABP
  memcpy(buf, APPEUI, 8);
  RevBytes(buf, 8); // TTN requires it in LSB First order, so we swap bytes
#endif
}

void os_getDevEui(u1_t *buf) {
#ifndef LORA_ABP
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
#endif
}

#if (VERBOSE)

// Display a key
void printKey(const char *name, const uint8_t *key, uint8_t len, bool lsb) {
  const uint8_t *p;
  char keystring[len + 1] = "", keybyte[3];
  for (uint8_t i = 0; i < len; i++) {
    p = lsb ? key + len - i - 1 : key + i;
    snprintf(keybyte, 3, "%02X", *p);
    strncat(keystring, keybyte, 2);
  }
  ESP_LOGI(TAG, "%s: %s", name, keystring);
}

// Display OTAA keys
void showLoraKeys(void) {
  // LMIC may not have used callback to fill
  // all EUI buffer so we do it here to a temp
  // buffer to be able to display them
  uint8_t buf[32];
  os_getArtEui((u1_t *)buf);
  printKey("AppEUI", buf, 8, true);
  os_getDevEui((u1_t *)buf);
  printKey("DevEUI", buf, 8, true);
  os_getDevKey((u1_t *)buf);
  printKey("AppKey", buf, 16, false);
}

#endif // VERBOSE

// LMIC send task
void lora_send(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  MessageBuffer_t SendBuffer;

  while (1) {
    // postpone until we are joined if we are not
    while (!LMIC.devaddr) {
      vTaskDelay(pdMS_TO_TICKS(500));
    }

    // fetch next or wait for payload to send from queue
    // do not delete item from queue until it is transmitted
    if (xQueuePeek(LoraSendQueue, &SendBuffer, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }

    // attempt to transmit payload
    switch (LMIC_setTxData2_strict(SendBuffer.MessagePort, SendBuffer.Message,
                                   SendBuffer.MessageSize,
                                   (cfg.countermode & 0x02))) {
    case LMIC_ERROR_SUCCESS:
#if (TIME_SYNC_LORASERVER)
      // if last packet sent was a timesync request, store TX timestamp
      if (SendBuffer.MessagePort == TIMEPORT)
        // store LMIC time when we started transmit of timesync request
        timesync_store(osticks2ms(os_getTime()), timesync_tx);
#endif
      ESP_LOGI(TAG, "%d byte(s) sent to LORA", SendBuffer.MessageSize);
      // delete sent item from queue
      xQueueReceive(LoraSendQueue, &SendBuffer, (TickType_t)0);
      break;
    case LMIC_ERROR_TX_BUSY:   // LMIC already has a tx message pending
    case LMIC_ERROR_TX_FAILED: // message was not sent
      vTaskDelay(pdMS_TO_TICKS(500 + random(400))); // wait a while
      break;
    case LMIC_ERROR_TX_TOO_LARGE:    // message size exceeds LMIC buffer size
    case LMIC_ERROR_TX_NOT_FEASIBLE: // message too large for current
                                     // datarate
      ESP_LOGI(TAG, "Message too large to send, message not sent and deleted");
      // we need some kind of error handling here -> to be done
      break;
    default: // other LMIC return code
      ESP_LOGE(TAG, "LMIC error, message not sent and deleted");
    }         // switch
    delay(2); // yield to CPU
  }           // while(1)
}

esp_err_t lmic_init(void) {
  _ASSERT(SEND_QUEUE_SIZE > 0);
  LoraSendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(MessageBuffer_t));
  if (LoraSendQueue == 0) {
    ESP_LOGE(TAG, "Could not create LORA send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "LORA send queue created, size %d Bytes",
           SEND_QUEUE_SIZE * sizeof(MessageBuffer_t));

  // setup LMIC stack
  os_init_ex(&myPinmap); // initialize lmic run-time environment

  // register a callback for downlink messages and lmic events.
  // We aren't trying to write reentrant code, so pUserData is NULL.
  // LMIC_reset() doesn't affect callbacks, so we can do this first.
  LMIC_registerRxMessageCb(myRxCallback, NULL);
  LMIC_registerEventCb(myEventCallback, NULL);
  // to come with future LMIC version

  // Reset the MAC state. Session and pending data transfers will be
  // discarded.
  LMIC_reset();

// This tells LMIC to make the receive windows bigger, in case your clock is
// faster or slower. This causes the transceiver to be earlier switched on,
// so consuming more power. You may sharpen (reduce) CLOCK_ERROR_PERCENTAGE
// in src/lmic_config.h if you are limited on battery.
#ifdef CLOCK_ERROR_PROCENTAGE
  LMIC_setClockError(CLOCK_ERROR_PROCENTAGE * MAX_CLOCK_ERROR / 1000);
#endif

// Pass ABP parameters to LMIC_setSession
#ifdef LORA_ABP
  setABPParameters(); // These parameters are defined as macro in loraconf.h

  // load saved session from RTC, if we have one
  if (RTC_runmode == RUNMODE_WAKEUP) {
    LoadLMICFromRTC();
  } else {
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(NETID, DEVADDR, nwkskey, appskey);
  }

  // Pass OTA parameters to LMIC_setSession
#else
  // load saved session from RTC, if we have one
  if (RTC_runmode == RUNMODE_WAKEUP)
    LoadLMICFromRTC();
  if (!LMIC_startJoining())
    ESP_LOGI(TAG, "Already joined");
#endif

  // start lmic loop task
  ESP_LOGI(TAG, "Starting LMIC...");
  xTaskCreatePinnedToCore(lmictask,   // task function
                          "lmictask", // name of task
                          4096,       // stack size of task
                          (void *)1,  // parameter of the task
                          2,          // priority of the task
                          &lmicTask,  // task handle
                          1);         // CPU core

  // start lora send task
  xTaskCreatePinnedToCore(lora_send,      // task function
                          "lorasendtask", // name of task
                          3072,           // stack size of task
                          (void *)1,      // parameter of the task
                          2,              // priority of the task
                          &lorasendTask,  // task handle
                          1);             // CPU core

  return ESP_OK;
}

void lora_enqueuedata(MessageBuffer_t *message) {
  // enqueue message in LORA send queue
  if (xQueueSendToBack(LoraSendQueue, (void *)message, (TickType_t)0) !=
      pdTRUE) {
    snprintf(lmic_event_msg + 14, LMIC_EVENTMSG_LEN - 14, "<>");
    ESP_LOGW(TAG, "LORA sendqueue is full");
  } else {
    // add Lora send queue length to display
    snprintf(lmic_event_msg + 14, LMIC_EVENTMSG_LEN - 14, "%2u",
             uxQueueMessagesWaiting(LoraSendQueue));
  }
}

void lora_queuereset(void) { xQueueReset(LoraSendQueue); }

uint32_t lora_queuewaiting(void) {
  return uxQueueMessagesWaiting(LoraSendQueue);
}

// LMIC loop task
void lmictask(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1);
  while (1) {
    os_runloop_once(); // execute lmic scheduled jobs and events
    delay(2);          // yield to CPU
  }
}

// lmic event handler
void myEventCallback(void *pUserData, ev_t ev) {
  // using message descriptors from LMIC library
  static const char *const evNames[] = {LMIC_EVENT_NAME_TABLE__INIT};
  // get current length of lora send queue
  uint8_t const msgWaiting = uxQueueMessagesWaiting(LoraSendQueue);

  // get current event message
  if (ev < sizeof(evNames) / sizeof(evNames[0]))
    snprintf(lmic_event_msg, LMIC_EVENTMSG_LEN, "%-16s",
             evNames[ev] + 3); // +3 to strip "EV_"
  else
    snprintf(lmic_event_msg, LMIC_EVENTMSG_LEN, "LMIC event %-4u ", ev);

  // process current event message
  switch (ev) {
  case EV_TXCOMPLETE:
    // -> processed in lora_send()
    break;

  case EV_RXCOMPLETE:
    // -> processed in myRxCallback()
    break;

  case EV_JOINING:
    // do the network-specific setup prior to join.
    lora_setupForNetwork(true);
    break;

  case EV_JOINED:
    // do the after join network-specific setup.
    lora_setupForNetwork(false);
    break;

  case EV_JOIN_FAILED:
    // must call LMIC_reset() to stop joining
    // otherwise join procedure continues.
    LMIC_reset();
    break;

  case EV_JOIN_TXCOMPLETE:
    // replace descriptor from library with more descriptive term
    snprintf(lmic_event_msg, LMIC_EVENTMSG_LEN, "%-16s", "JOIN_WAIT");
    break;

  default:
    break;
  }

  // add Lora send queue length to display
  if (msgWaiting)
    snprintf(lmic_event_msg + 14, LMIC_EVENTMSG_LEN - 14, "%2u", msgWaiting);

  // print event
  ESP_LOGD(TAG, "%s", lmic_event_msg);
}

// event EV_RXCOMPLETE message handler
void myRxCallback(void *pUserData, uint8_t port, const uint8_t *pMsg,
                  size_t nMsg) {
  // display amount of received data
  if (nMsg)
    ESP_LOGI(TAG, "Received %u byte(s) of payload on port %u", nMsg, port);
  else if (port)
    ESP_LOGI(TAG, "Received empty message on port %u", port);

  switch (port) {
  // rcommand received -> call interpreter
  case RCMDPORT:
    rcommand(pMsg, nMsg);
    break;

// timeserver answer -> call timesync processor
#if (TIME_SYNC_LORASERVER)
  case TIMEPORT:
    // get and store gwtime from payload
    timesync_serverAnswer(const_cast<uint8_t *>(pMsg), nMsg);
    break;
#endif
  } // switch
}

const char *getSfName(rps_t rps) {
  const char *const t[] = {"FSK",  "SF7",  "SF8",  "SF9",
                           "SF10", "SF11", "SF12", "SF?"};
  return t[getSf(rps)];
}

const char *getBwName(rps_t rps) {
  const char *const t[] = {"BW125", "BW250", "BW500", "BW?"};
  return t[getBw(rps)];
}

const char *getCrName(rps_t rps) {
  const char *const t[] = {"CR 4/5", "CR 4/6", "CR 4/7", "CR 4/8"};
  return t[getCr(rps)];
}

/*******************************************************************************
 *
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 *
 * Copyright (c) 2018-2021 Manuel Bleichenbacher
 *
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * Functions for storing and retrieving TTN communication state from RTC memory.
 *******************************************************************************/

#define LMIC_OFFSET(field) __builtin_offsetof(struct lmic_t, field)
#define LMIC_DIST(field1, field2) (LMIC_OFFSET(field2) - LMIC_OFFSET(field1))
#define TTN_RTC_MEM_SIZE                                                       \
  (sizeof(struct lmic_t) - LMIC_OFFSET(radio) - MAX_LEN_PAYLOAD - MAX_LEN_FRAME)

#define TTN_RTC_FLAG_VALUE 0xf8025b8a

RTC_DATA_ATTR uint8_t ttn_rtc_mem_buf[TTN_RTC_MEM_SIZE];
RTC_DATA_ATTR uint32_t ttn_rtc_flag;

void ttn_rtc_save() {
  // Copy LMIC struct except client, osjob, pendTxData and frame
  size_t len1 = LMIC_DIST(radio, pendTxData);
  memcpy(ttn_rtc_mem_buf, &LMIC.radio, len1);
  size_t len2 = LMIC_DIST(pendTxData, frame) - MAX_LEN_PAYLOAD;
  memcpy(ttn_rtc_mem_buf + len1, (u1_t *)&LMIC.pendTxData + MAX_LEN_PAYLOAD,
         len2);
  size_t len3 = sizeof(struct lmic_t) - LMIC_OFFSET(frame) - MAX_LEN_FRAME;
  memcpy(ttn_rtc_mem_buf + len1 + len2, (u1_t *)&LMIC.frame + MAX_LEN_FRAME,
         len3);

  ttn_rtc_flag = TTN_RTC_FLAG_VALUE;
}

bool ttn_rtc_restore() {
  if (ttn_rtc_flag != TTN_RTC_FLAG_VALUE)
    return false;

  // Restore data
  size_t len1 = LMIC_DIST(radio, pendTxData);
  memcpy(&LMIC.radio, ttn_rtc_mem_buf, len1);
  memset(LMIC.pendTxData, 0, MAX_LEN_PAYLOAD);
  size_t len2 = LMIC_DIST(pendTxData, frame) - MAX_LEN_PAYLOAD;
  memcpy((u1_t *)&LMIC.pendTxData + MAX_LEN_PAYLOAD, ttn_rtc_mem_buf + len1,
         len2);
  memset(LMIC.frame, 0, MAX_LEN_FRAME);
  size_t len3 = sizeof(struct lmic_t) - LMIC_OFFSET(frame) - MAX_LEN_FRAME;
  memcpy((u1_t *)&LMIC.frame + MAX_LEN_FRAME, ttn_rtc_mem_buf + len1 + len2,
         len3);

  ttn_rtc_flag = 0xffffffff; // invalidate RTC data

  return true;
}

// following code includes snippets taken from
// https://github.com/JackGruber/ESP32-LMIC-DeepSleep-example/blob/master/src/main.cpp

void SaveLMICToRTC(uint32_t deepsleep_sec) {
  // ESP32 can't track millis during DeepSleep and no option to advance
  // millis after DeepSleep. Therefore reset DutyCyles before saving LMIC struct

  unsigned long now = millis();

  // EU Like Bands
#if CFG_LMIC_EU_like
  for (int i = 0; i < MAX_BANDS; i++) {
    ostime_t correctedAvail =
        LMIC.bands[i].avail -
        ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
    if (correctedAvail < 0) {
      correctedAvail = 0;
    }
    LMIC.bands[i].avail = correctedAvail;
  }

  LMIC.globalDutyAvail =
      LMIC.globalDutyAvail - ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
  if (LMIC.globalDutyAvail < 0) {
    LMIC.globalDutyAvail = 0;
  }
#else
  ESP_LOGW(TAG, "No DutyCycle recalculation function!");
#endif

  ttn_rtc_save();
  ESP_LOGI(TAG, "LMIC state saved");
}

void LoadLMICFromRTC() {
  if (ttn_rtc_restore())
    ESP_LOGI(TAG, "LMIC state loaded");
  else {
    ESP_LOGE(TAG, "LMIC state not found - resetting device");
    do_reset(false); // coldstart
  }
}

#endif // HAS_LORA