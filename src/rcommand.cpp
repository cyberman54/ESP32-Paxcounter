// Basic Config
#include "globals.h"
#include "rcommand.h"

// Local logging tag
static const char TAG[] = __FILE__;

// helper function
void do_reset() {
  ESP_LOGI(TAG, "Remote command: restart device");
#if (HAS_LORA)
  LMIC_shutdown();
#endif
  delay(3000);
  esp_restart();
}

// set of functions that can be triggered by remote commands
void set_reset(uint8_t val[]) {
  switch (val[0]) {
  case 0: // restart device
    sprintf(display_line6, "Reset pending");
    do_reset();
    break;
  case 1: // reset MAC counter
    ESP_LOGI(TAG, "Remote command: reset MAC counter");
    reset_counters(); // clear macs
    get_salt();       // get new salt
    sprintf(display_line6, "Reset counter");
    break;
  case 2: // reset device to factory settings
    ESP_LOGI(TAG, "Remote command: reset device to factory settings");
    sprintf(display_line6, "Factory reset");
    eraseConfig();
    break;
  case 3: // reset send queues
    ESP_LOGI(TAG, "Remote command: flush send queue");
    sprintf(display_line6, "Queue reset");
    flushQueues();
    break;
  case 9: // reset and ask for software update via Wifi OTA
    ESP_LOGI(TAG, "Remote command: software update via Wifi");
#if (USE_OTA)
    sprintf(display_line6, "Software update");
    cfg.runmode = 1;
#else
    sprintf(display_line6, "Software update not implemented");
#endif // USE_OTA
    break;

  default:
    ESP_LOGW(TAG, "Remote command: reset called with invalid parameter(s)");
  }
}

void set_rssi(uint8_t val[]) {
  cfg.rssilimit = val[0] * -1;
  ESP_LOGI(TAG, "Remote command: set RSSI limit to %d", cfg.rssilimit);
}

void set_sendcycle(uint8_t val[]) {
  cfg.sendcycle = val[0];
  // update send cycle interrupt [seconds
  sendcycler.attach(cfg.sendcycle * 2, sendcycle);
  ESP_LOGI(TAG, "Remote command: set send cycle to %d seconds",
           cfg.sendcycle * 2);
}

void set_wifichancycle(uint8_t val[]) {
  cfg.wifichancycle = val[0];
  // update Wifi channel rotation timer period
  xTimerChangePeriod(WifiChanTimer, pdMS_TO_TICKS(cfg.wifichancycle * 10), 100);

  ESP_LOGI(TAG,
           "Remote command: set Wifi channel switch interval to %.1f seconds",
           cfg.wifichancycle / float(100));
}

void set_blescantime(uint8_t val[]) {
  cfg.blescantime = val[0];
  ESP_LOGI(TAG, "Remote command: set BLE scan time to %.1f seconds",
           cfg.blescantime / float(100));
  // stop & restart BLE scan task to apply new parameter
  if (cfg.blescan) {
    stop_BLEscan();
    start_BLEscan();
  }
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
  reset_counters(); // clear macs
  get_salt();       // get new salt
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
  switch (val[0]) { // check if valid sensor number 1...4
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

void set_beacon(uint8_t val[]) {
  uint8_t id = val[0];           // use first parameter as beacon storage id
  memmove(val, val + 1, 6);      // strip off storage id
  beacons[id] = macConvert(val); // store beacon MAC in array
  ESP_LOGI(TAG, "Remote command: set beacon ID#%d", id);
  printKey("MAC", val, 6, false); // show beacon MAC
}

void set_monitor(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set beacon monitor mode to %s",
           val ? "on" : "off");
  cfg.monitormode = val[0] ? 1 : 0;
}

void set_lorasf(uint8_t val[]) {
#if (HAS_LORA)
  ESP_LOGI(TAG, "Remote command: set LoRa SF to %d", val[0]);
  switch_lora(val[0], cfg.txpower);
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
  if (cfg.blescan)
    start_BLEscan();
  else {
    macs_ble = 0; // clear BLE counter
    stop_BLEscan();
  }
}

void set_wifiant(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set Wifi antenna to %s",
           val[0] ? "external" : "internal");
  cfg.wifiant = val[0] ? 1 : 0;
#ifdef HAS_ANTENNA_SWITCH
  antenna_select(cfg.wifiant);
#endif
}

void set_vendorfilter(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set vendorfilter mode to %s",
           val[0] ? "on" : "off");
  cfg.vendorfilter = val[0] ? 1 : 0;
}

void set_rgblum(uint8_t val[]) {
  // Avoid wrong parameters
  cfg.rgblum = (val[0] >= 0 && val[0] <= 100) ? (uint8_t)val[0] : RGBLUMINOSITY;
  ESP_LOGI(TAG, "Remote command: set RGB Led luminosity %d", cfg.rgblum);
};

void set_lorapower(uint8_t val[]) {
#if (HAS_LORA)
  ESP_LOGI(TAG, "Remote command: set LoRa TXPOWER to %d", val[0]);
  switch_lora(cfg.lorasf, val[0]);
#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
};

void get_config(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get device configuration");
  payload.reset();
  payload.addConfig(cfg);
  SendPayload(CONFIGPORT, prio_high);
};

void get_status(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get device status");
  payload.reset();
  payload.addStatus(read_voltage(), uptime() / 1000, temperatureRead(),
                    getFreeRAM(), rtc_get_reset_reason(0),
                    rtc_get_reset_reason(1));
  SendPayload(STATUSPORT, prio_high);
};

void get_gps(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get gps status");
#if (HAS_GPS)
  gpsStatus_t gps_status;
  gps_storelocation(&gps_status);
  payload.reset();
  payload.addGPS(gps_status);
  SendPayload(GPSPORT, prio_high);
#else
  ESP_LOGW(TAG, "GPS function not supported");
#endif
};

void get_bme(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get bme680 sensor data");
#if (HAS_BME)
  payload.reset();
  payload.addBME(bme_status);
  SendPayload(BMEPORT, prio_high);
#else
  ESP_LOGW(TAG, "BME sensor not supported");
#endif
};

void get_batt(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get battery voltage");
#if (defined BAT_MEASURE_ADC || defined HAS_PMU)
  payload.reset();
  payload.addVoltage(read_voltage());
  SendPayload(BATTPORT, prio_normal);
#else
  ESP_LOGW(TAG, "Battery voltage not supported");
#endif
};

void get_time(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get time");
  payload.reset();
  payload.addTime(now());
  payload.addByte(timeStatus() << 4 | timeSource);
  SendPayload(TIMEPORT, prio_high);
};

void set_time(uint8_t val[]) {
  ESP_LOGI(TAG, "Timesync requested by timeserver");
  timeSync();
};

void set_flush(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: flush");
  // does nothing
  // used to open receive window on LoRaWAN class a nodes
};

// assign previously defined functions to set of numeric remote commands
// format: opcode, function, #bytes params,
// flag (true = do make settings persistent / false = don't)
//
static cmd_t table[] = {
    {0x01, set_rssi, 1, true},          {0x02, set_countmode, 1, true},
    {0x03, set_gps, 1, true},           {0x04, set_display, 1, true},
    {0x05, set_lorasf, 1, true},        {0x06, set_lorapower, 1, true},
    {0x07, set_loraadr, 1, true},       {0x08, set_screensaver, 1, true},
    {0x09, set_reset, 1, true},         {0x0a, set_sendcycle, 1, true},
    {0x0b, set_wifichancycle, 1, true}, {0x0c, set_blescantime, 1, true},
    {0x0d, set_vendorfilter, 1, false}, {0x0e, set_blescan, 1, true},
    {0x0f, set_wifiant, 1, true},       {0x10, set_rgblum, 1, true},
    {0x11, set_monitor, 1, true},       {0x12, set_beacon, 7, false},
    {0x13, set_sensor, 2, true},        {0x14, set_payloadmask, 1, true},
    {0x15, set_bme, 1, true},           {0x16, set_batt, 1, true},
    {0x80, get_config, 0, false},       {0x81, get_status, 0, false},
    {0x83, get_batt, 0, false},         {0x84, get_gps, 0, false},
    {0x85, get_bme, 0, false},          {0x86, get_time, 0, false},
    {0x87, set_time, 0, false},         {0x99, set_flush, 0, false}};

static const uint8_t cmdtablesize =
    sizeof(table) / sizeof(table[0]); // number of commands in command table

// check and execute remote command
void rcommand(const uint8_t cmd[], const uint8_t cmdlength) {

  if (cmdlength == 0)
    return;

  uint8_t foundcmd[cmdlength], cursor = 0;
  bool storeflag = false;

  while (cursor < cmdlength) {

    int i = cmdtablesize;
    while (i--) {
      if (cmd[cursor] == table[i].opcode) { // lookup command in opcode table
        cursor++;                           // strip 1 byte opcode
        if ((cursor + table[i].params) <= cmdlength) {
          memmove(foundcmd, cmd + cursor,
                  table[i].params); // strip opcode from cmd array
          cursor += table[i].params;
          if (table[i].store) // ceck if function needs to store configuration
            storeflag = true;
          table[i].func(
              foundcmd); // execute assigned function with given parameters
        } else
          ESP_LOGI(
              TAG,
              "Remote command x%02X called with missing parameter(s), skipped",
              table[i].opcode);
        break;   // command found -> exit table lookup loop
      }          // end of command validation
    }            // end of command table lookup loop
    if (i < 0) { // command not found -> exit parser
      ESP_LOGI(TAG, "Unknown remote command x%02X, ignored", cmd[cursor]);
      break;
    }
  } // command parsing loop

  if (storeflag)
    saveConfig();
} // rcommand()
