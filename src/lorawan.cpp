#ifdef HAS_LORA

// Basic Config
#include "lorawan.h"

// Local logging Tag
static const char TAG[] = "lora";

osjob_t sendjob;
QueueHandle_t LoraSendQueue;

// LMIC enhanced Pin mapping
const lmic_pinmap lmic_pins = {.mosi = PIN_SPI_MOSI,
                               .miso = PIN_SPI_MISO,
                               .sck = PIN_SPI_SCK,
                               .nss = PIN_SPI_SS,
                               .rxtx = LMIC_UNUSED_PIN,
                               .rst = RST,
                               .dio = {DIO0, DIO1, DIO2}};

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
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
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

#ifdef VERBOSE

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
  case EV_LOST_TSYNC:
    strcpy_P(buff, PSTR("LOST TSYNC"));
    break;
  case EV_RESET:
    strcpy_P(buff, PSTR("RESET"));
    break;
  case EV_RXCOMPLETE:
    strcpy_P(buff, PSTR("RX COMPLETE"));
    break;
  case EV_LINK_DEAD:
    strcpy_P(buff, PSTR("LINK DEAD"));
    break;
  case EV_LINK_ALIVE:
    strcpy_P(buff, PSTR("LINK ALIVE"));
    break;
  case EV_RFU1:
    strcpy_P(buff, PSTR("RFUI"));
    break;
  case EV_JOIN_FAILED:
    strcpy_P(buff, PSTR("JOIN FAILED"));
    break;
  case EV_REJOIN_FAILED:
    strcpy_P(buff, PSTR("REJOIN FAILED"));
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
    ESP_LOGI(TAG, "ADR=%d, SF=%d, TXPOWER=%d", cfg.adrmode, cfg.lorasf,
             cfg.txpower);
    break;

  case EV_TXCOMPLETE:

    strcpy_P(buff, (LMIC.txrxFlags & TXRX_ACK) ? PSTR("RECEIVED ACK")
                                               : PSTR("TX COMPLETE"));
    sprintf(display_line6, " "); // clear previous lmic status

    if (LMIC.dataLen) {
      ESP_LOGI(TAG, "Received %d bytes of payload, RSSI %d SNR %d",
               LMIC.dataLen, LMIC.rssi, (signed char)LMIC.snr);
      sprintf(display_line6, "RSSI %d SNR %d", LMIC.rssi,
              (signed char)LMIC.snr);

      // check if command is received on command port, then call interpreter
      if ((LMIC.txrxFlags & TXRX_PORT) &&
          (LMIC.frame[LMIC.dataBeg - 1] == RCMDPORT))
        rcommand(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
    }
    break;

  default:
    sprintf_P(buff, PSTR("UNKNOWN EVENT %d"), ev);
    break;
  }

  // Log & Display if asked
  if (*buff) {
    ESP_LOGI(TAG, "EV_%s", buff);
    sprintf(display_line7, buff);
  }

} // onEvent()

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
#if defined(CFG_eu868)
    LMIC_setDrTxpow(DR_SF11, tx);
    cfg.lorasf = sf;
    break;
#elif defined(CFG_us915)
    LMIC_setDrTxpow(DR_SF11CR, tx);
    cfg.lorasf = sf;
    break;
#endif
  case 12:
#if defined(CFG_eu868)
    LMIC_setDrTxpow(DR_SF12, tx);
    cfg.lorasf = sf;
    break;
#elif defined(CFG_us915)
    LMIC_setDrTxpow(DR_SF12CR, tx);
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
      // SendBuffer gets struct MessageBuffer with next payload from queue
      LMIC_setTxData2(SendBuffer.MessagePort, SendBuffer.Message,
                      SendBuffer.MessageSize, (cfg.countermode & 0x02));
      ESP_LOGI(TAG, "%d bytes sent to LoRa", SendBuffer.MessageSize);
      sprintf(display_line7, "PACKET QUEUED");
    }
  }
  // reschedule job every 0,5 - 1 sec. including a bit of random to prevent
  // systematic collisions
  os_setTimedCallback(job, os_getTime() + 500 + ms2osticks(random(500)),
                      lora_send);
}

#endif // HAS_LORA