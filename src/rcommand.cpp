// Basic Config
#include "globals.h"
#include "rcommand.h"


static QueueHandle_t RcmdQueue;
TaskHandle_t rcmdTask;

// set of functions that can be triggered by remote commands
void set_reset(uint8_t val[]) {
  switch (val[0]) {
  case 0: // restart device with cold start (clear RTC saved variables)
    ESP_LOGI(TAG, "Remote command: restart device cold");
    do_reset(false);
    break;
  case 1: // reserved
    // reset MAC counter deprecated by libpax integration
    break;
  case 2: // reset device to factory settings
    ESP_LOGI(TAG,
             "Remote command: reset device to factory settings and restart");
    eraseConfig();
    do_reset(false);
    break;
  case 3: // reset send queues
    ESP_LOGI(TAG, "Remote command: flush send queue");
    flushQueues();
    break;
  case 4: // restart device with warm start (keep RTC saved variables)
    ESP_LOGI(TAG, "Remote command: restart device warm");
    do_reset(true);
    break;
  case 8: // reset and start local web server for manual software update
    ESP_LOGI(TAG, "Remote command: reboot to maintenance mode");
    RTC_runmode = RUNMODE_MAINTENANCE;
    break;
  case 9: // reset and ask OTA server via Wifi for automated software update
    ESP_LOGI(TAG, "Remote command: reboot to ota update mode");
#if (USE_OTA)
    // check power status before scheduling ota update
    if (batt_sufficient())
      RTC_runmode = RUNMODE_UPDATE;
    else
      ESP_LOGE(TAG, "Battery level %d%% is too low for OTA", batt_level);
#endif // USE_OTA
    break;

  default:
    ESP_LOGW(TAG, "Remote command: reset called with invalid parameter(s)");
  }
}

void set_rssi(uint8_t val[]) {
  cfg.rssilimit = val[0] * -1;
  libpax_counter_stop();
  libpax_config_t current_config;
  libpax_get_current_config(&current_config);
  current_config.wifi_rssi_threshold = cfg.rssilimit;
  current_config.ble_rssi_threshold = cfg.rssilimit;
  libpax_update_config(&current_config);
  init_libpax();
  ESP_LOGI(TAG, "Remote command: set RSSI limit to %d", cfg.rssilimit);
}

void set_sendcycle(uint8_t val[]) {
  if (val[0] < 5)
    return;
  // update send cycle interrupt [seconds / 2]
  cfg.sendcycle = val[0];
  ESP_LOGI(TAG, "Remote command: set send cycle to %d seconds",
           cfg.sendcycle * 2);
  libpax_counter_stop();
  init_libpax();
}

void set_sleepcycle(uint8_t val[]) {
  // swap byte order from msb to lsb, note: this is a platform dependent hack
  cfg.sleepcycle = __builtin_bswap16(*(uint16_t *)(val));
  ESP_LOGI(TAG, "Remote command: set sleep cycle to %d seconds",
           cfg.sleepcycle * 10);
}

void set_wifichancycle(uint8_t val[]) {
  cfg.wifichancycle = val[0];
  libpax_counter_stop();
  libpax_config_t current_config;
  libpax_get_current_config(&current_config);

  if (cfg.wifichancycle == 0) {
    ESP_LOGI(TAG, "Remote command: set Wifi channel hopping to off");
    current_config.wifi_channel_map = WIFI_CHANNEL_1;
  } else {
    ESP_LOGI(
        TAG,
        "Remote command: set Wifi channel hopping interval to %.1f seconds",
        cfg.wifichancycle / float(100));
  }

  current_config.wifi_channel_switch_interval = cfg.wifichancycle;
  libpax_update_config(&current_config);
  init_libpax();
}

void set_blescantime(uint8_t val[]) {
  cfg.blescantime = val[0];
  libpax_counter_stop();
  libpax_config_t current_config;
  libpax_get_current_config(&current_config);
  current_config.blescantime = cfg.blescantime;
  libpax_update_config(&current_config);
  init_libpax();
}

void set_countmode(uint8_t val[]) {
  switch (val[0]) {
  case 0: // cyclic unconfirmed
    cfg.countermode = 0;
    ESP_LOGI(TAG, "Remote command: set counter mode to cyclic unconfirmed");
    break;
  case 1: // cumulative
    cfg.countermode = 1;
    ESP_LOGI(TAG, "Remote command: set counter mode to cumulative");
    break;
  case 2: // cyclic confirmed
    cfg.countermode = 2;
    ESP_LOGI(TAG, "Remote command: set counter mode to cyclic confirmed");
    break;
  default: // invalid parameter
    ESP_LOGW(
        TAG,
        "Remote command: set counter mode called with invalid parameter(s)");
    return;
  }
  libpax_counter_stop();
  init_libpax(); // re-inits counter mode from cfg.countermode
}

void set_screensaver(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set screen saver to %s ",
           val[0] ? "on" : "off");
  cfg.screensaver = val[0] ? 1 : 0;
}

void set_display(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set screen to %s", val[0] ? "on" : "off");
  cfg.screenon = val[0] ? 1 : 0;
}

void set_gps(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set GPS mode to %s", val[0] ? "on" : "off");
  if (val[0]) {
    cfg.payloadmask |= (uint8_t)GPS_DATA; // set bit in mask
  } else {
    cfg.payloadmask &= (uint8_t)~GPS_DATA; // clear bit in mask
  }
}

void set_bme(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set BME mode to %s", val[0] ? "on" : "off");
  if (val[0]) {
    cfg.payloadmask |= (uint8_t)MEMS_DATA; // set bit in mask
  } else {
    cfg.payloadmask &= (uint8_t)~MEMS_DATA; // clear bit in mask
  }
}

void set_batt(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set battery mode to %s",
           val[0] ? "on" : "off");
  if (val[0]) {
    cfg.payloadmask |= (uint8_t)BATT_DATA; // set bit in mask
  } else {
    cfg.payloadmask &= (uint8_t)~BATT_DATA; // clear bit in mask
  }
}

void set_payloadmask(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set payload mask to %X", val[0]);
  cfg.payloadmask = val[0];
}

void set_sensor(uint8_t val[]) {
#if (HAS_SENSORS)
  switch (val[0]) { // check if valid sensor number 1..3
  case 1:
  case 2:
  case 3:
    break; // valid sensor number -> continue
  default:
    ESP_LOGW(
        TAG,
        "Remote command set sensor mode called with invalid sensor number");
    return; // invalid sensor number -> exit
  }

  ESP_LOGI(TAG, "Remote command: set sensor #%d mode to %s", val[0],
           val[1] ? "on" : "off");

  if (val[1])
    cfg.payloadmask |= sensor_mask(val[0]); // set bit
  else
    cfg.payloadmask &= ~sensor_mask(val[0]); // clear bit
#endif
}

uint64_t macConvert(uint8_t *paddr) {
  uint64_t *mac;
  mac = (uint64_t *)paddr;
  return (__builtin_bswap64(*mac) >> 16);
}

void set_loradr(uint8_t val[]) {
#if (HAS_LORA)
  if (validDR(val[0])) {
    cfg.loradr = val[0];
    ESP_LOGI(TAG, "Remote command: set LoRa Datarate to %d", cfg.loradr);
    LMIC_setDrTxpow(assertDR(cfg.loradr), KEEP_TXPOW);
    ESP_LOGI(TAG, "Radio parameters now %s / %s / %s",
             getSfName(updr2rps(LMIC.datarate)),
             getBwName(updr2rps(LMIC.datarate)),
             getCrName(updr2rps(LMIC.datarate)));
  } else
    ESP_LOGI(
        TAG,
        "Remote command: set LoRa Datarate called with illegal datarate %d",
        val[0]);
#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
}

void set_loraadr(uint8_t val[]) {
#if (HAS_LORA)
  ESP_LOGI(TAG, "Remote command: set LoRa ADR mode to %s",
           val[0] ? "on" : "off");
  cfg.adrmode = val[0] ? 1 : 0;
  LMIC_setAdrMode(cfg.adrmode);
#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
}

void set_blescan(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set BLE scanner to %s", val[0] ? "on" : "off");
  cfg.blescan = val[0] ? 1 : 0;
  libpax_counter_stop();
  libpax_config_t current_config;
  libpax_get_current_config(&current_config);
  current_config.blecounter = cfg.blescan;
  libpax_update_config(&current_config);
  init_libpax();
}

void set_wifiscan(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set WIFI scanner to %s",
           val[0] ? "on" : "off");
  cfg.wifiscan = val[0] ? 1 : 0;
  libpax_counter_stop();
  libpax_config_t current_config;
  libpax_get_current_config(&current_config);
  current_config.wificounter = cfg.wifiscan;
  libpax_update_config(&current_config);
  init_libpax();
}

void set_wifiant(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set Wifi antenna to %s",
           val[0] ? "external" : "internal");
  cfg.wifiant = val[0] ? 1 : 0;
#ifdef HAS_ANTENNA_SWITCH
  antenna_select(cfg.wifiant);
#endif
}

void set_rgblum(uint8_t val[]) {
  // Avoid wrong parameters
  cfg.rgblum = (val[0] <= 100) ? (uint8_t)val[0] : RGBLUMINOSITY;
  ESP_LOGI(TAG, "Remote command: set RGB Led luminosity %d", cfg.rgblum);
};

void set_lorapower(uint8_t val[]) {
#if (HAS_LORA)
  // set data rate and transmit power only if we have no ADR
  if (!cfg.adrmode) {
    cfg.txpower = val[0];
    ESP_LOGI(TAG, "Remote command: set LoRa TXPOWER to %d", cfg.txpower);
    LMIC_setDrTxpow(assertDR(cfg.loradr), cfg.txpower);
  } else
    ESP_LOGI(
        TAG,
        "Remote command: set LoRa TXPOWER, not executed because ADR is on");

#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
};

void get_config(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get device configuration");
  payload.reset();
  payload.addConfig(cfg);
  SendPayload(CONFIGPORT);
};

void get_status(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get device status");
  payload.reset();
#ifdef CONFIG_IDF_TARGET_ESP32S3
  payload.addStatus(read_voltage(), (uint64_t)(uptime() / 1000ULL), 0,
                    // temperatureRead(),
                    getFreeRAM(), rtc_get_reset_reason(0), RTC_restarts);
#else
  payload.addStatus(read_voltage(), (uint64_t)(uptime() / 1000ULL),
                    temperatureRead(), getFreeRAM(), rtc_get_reset_reason(0),
                    RTC_restarts);
#endif
  SendPayload(STATUSPORT);
};

void get_gps(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get gps status");
#if (HAS_GPS)
  gpsStatus_t gps_status;
  gps_storelocation(&gps_status);
  payload.reset();
  payload.addGPS(gps_status);
  SendPayload(GPSPORT);
#else
  ESP_LOGW(TAG, "GPS function not supported");
#endif
};

void get_bme(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get bme680 sensor data");
#if (HAS_BME)
  payload.reset();
  payload.addBME(bme_status);
  SendPayload(BMEPORT);
#else
  ESP_LOGW(TAG, "BME sensor not supported");
#endif
};

void get_batt(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get battery voltage");
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
  payload.reset();
  payload.addVoltage(read_voltage());
  SendPayload(BATTPORT);
#else
  ESP_LOGW(TAG, "Battery voltage not supported");
#endif
};

void get_time(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get time");
  time_t t = time(NULL);
  payload.reset();
  payload.addTime(t);
  payload.addByte(sntp_get_sync_status() << 4 | timeSource);
  SendPayload(TIMEPORT);
};

void set_timesync(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: timesync requested");
  setTimeSyncIRQ();
};

void set_time(uint8_t val[]) {
  // swap byte order from msb to lsb, note: this is a platform dependent hack
  uint32_t t = __builtin_bswap32(*(uint32_t *)(val));
  ESP_LOGI(TAG, "Remote command: set time to %d", t);
  setMyTime(t, 0, _set);
};

void set_flush(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: flush");
  // does nothing
  // used to open receive window on LoRaWAN class a nodes
};

void set_loadconfig(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: load config from NVRAM");
  loadConfig();
};

void set_saveconfig(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: save config to NVRAM");
  saveConfig(false);
};

// assign previously defined functions to set of numeric remote commands
// format: {opcode, function, number of function arguments}

static const cmd_t table[] = {
    {0x01, set_rssi, 1},          {0x02, set_countmode, 1},
    {0x03, set_gps, 1},           {0x04, set_display, 1},
    {0x05, set_loradr, 1},        {0x06, set_lorapower, 1},
    {0x07, set_loraadr, 1},       {0x08, set_screensaver, 1},
    {0x09, set_reset, 1},         {0x0a, set_sendcycle, 1},
    {0x0b, set_wifichancycle, 1}, {0x0c, set_blescantime, 1},
    {0x0e, set_blescan, 1},       {0x0f, set_wifiant, 1},
    {0x10, set_rgblum, 1},        {0x13, set_sensor, 2},
    {0x14, set_payloadmask, 1},   {0x15, set_bme, 1},
    {0x16, set_batt, 1},          {0x17, set_wifiscan, 1},
    {0x18, set_flush, 0},         {0x19, set_sleepcycle, 2},
    {0x20, set_loadconfig, 0},    {0x21, set_saveconfig, 0},
    {0x80, get_config, 0},        {0x81, get_status, 0},
    {0x83, get_batt, 0},          {0x84, get_gps, 0},
    {0x85, get_bme, 0},           {0x86, get_time, 0},
    {0x87, set_timesync, 0},      {0x88, set_time, 4},
    {0x99, set_flush, 0}};

static const uint8_t cmdtablesize =
    sizeof(table) / sizeof(table[0]); // number of commands in command table

// check and execute remote command
void rcmd_execute(const uint8_t cmd[], const uint8_t cmdlength) {
  if (cmdlength == 0)
    return;

  uint8_t foundcmd[cmdlength], cursor = 0;

  while (cursor < cmdlength) {
    int i = cmdtablesize;
    while (i--) {
      if (cmd[cursor] == table[i].opcode) { // lookup command in opcode table
        cursor++;                           // strip 1 byte opcode
        if ((cursor + table[i].params) <= cmdlength) {
          memmove(foundcmd, cmd + cursor,
                  table[i].params); // strip opcode from cmd array
          cursor += table[i].params;
          table[i].func(
              foundcmd); // execute assigned function with given parameters
        } else
          ESP_LOGI(TAG,
                   "Remote command x%02X called with missing parameter(s), "
                   "skipped",
                   table[i].opcode);
        break; // command found -> exit table lookup loop
      }        // end of command validation
    }          // end of command table lookup loop

    if (i < 0) { // command not found -> exit parser
      ESP_LOGI(TAG, "Unknown remote command x%02X, ignored", cmd[cursor]);
      break;
    }
  } // command parsing loop
} //  rcmd_execute()

// remote command processing task
void rcmd_process(void *pvParameters) {
  _ASSERT((uint32_t)pvParameters == 1); // FreeRTOS check

  RcmdBuffer_t RcmdBuffer;

  while (1) {
    // fetch next or wait for incoming rcommand from queue
    if (xQueueReceive(RcmdQueue, &RcmdBuffer, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "Premature return from xQueueReceive() with no data!");
      continue;
    }
    rcmd_execute(RcmdBuffer.cmd, RcmdBuffer.cmdLen);
  }

  delay(5); // yield to CPU
} // rcmd_process()

// enqueue remote command
void rcommand(const uint8_t *cmd, const size_t cmdlength) {
  RcmdBuffer_t rcmd = {0};
  rcmd.cmdLen = cmdlength;
  memcpy(rcmd.cmd, cmd, cmdlength);

  if (xQueueSendToBack(RcmdQueue, (void *)&rcmd, (TickType_t)0) != pdTRUE)
    ESP_LOGW(TAG, "Remote command queue is full");
} // rcommand()

void rcmd_queuereset(void) { xQueueReset(RcmdQueue); }

uint32_t rcmd_queuewaiting(void) { return uxQueueMessagesWaiting(RcmdQueue); }

void rcmd_deinit(void) {
  rcmd_queuereset();
  vTaskDelete(rcmdTask);
}

esp_err_t rcmd_init(void) {
  _ASSERT(RCMD_QUEUE_SIZE > 0);
  RcmdQueue = xQueueCreate(RCMD_QUEUE_SIZE, sizeof(RcmdBuffer_t));
  if (RcmdQueue == 0) {
    ESP_LOGE(TAG, "Could not create rcommand send queue. Aborting.");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Rcommand send queue created, size %d Bytes",
           RCMD_QUEUE_SIZE * sizeof(RcmdBuffer_t));

  xTaskCreatePinnedToCore(rcmd_process, // task function
                          "rcmdloop",   // name of task
                          3072,         // stack size of task
                          (void *)1,    // parameter of the task
                          1,            // priority of the task
                          &rcmdTask,    // task handle
                          1);           // CPU core

  return ESP_OK;
} // rcmd_init()