// remote command interpreter, parses and executes commands with arguments in
// array

// Basic Config
#include "globals.h"
#include "rcommand.h"

// Local logging tag
static const char TAG[] = "main";

#ifdef HAS_LORA
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
#endif // HAS_LORA

// set of functions that can be triggered by remote commands
void set_reset(uint8_t val[]) {
  switch (val[0]) {
  case 0: // restart device
    ESP_LOGI(TAG, "Remote command: restart device");
    sprintf(display_line6, "Reset pending");
    vTaskDelay(10000 / portTICK_PERIOD_MS); // wait for LMIC to confirm LoRa
                                            // downlink to server
    esp_restart();
    break;
  case 1: // reset MAC counter
    ESP_LOGI(TAG, "Remote command: reset MAC counter");
    reset_counters(); // clear macs
    reset_salt();     // get new salt
    sprintf(display_line6, "Reset counter");
    break;
  case 2: // reset device to factory settings
    ESP_LOGI(TAG, "Remote command: reset device to factory settings");
    sprintf(display_line6, "Factory reset");
    eraseConfig();
    break;
  default:
    ESP_LOGW(TAG, "Remote command: reset called with invalid parameter(s)");
  }
};

void set_rssi(uint8_t val[]) {
  cfg.rssilimit = val[0] * -1;
  ESP_LOGI(TAG, "Remote command: set RSSI limit to %d", cfg.rssilimit);
};

void set_sendcycle(uint8_t val[]) {
  cfg.sendcycle = val[0];
  // update send cycle interrupt
  timerAlarmWrite(sendCycle, cfg.sendcycle * 2 * 10000, true);
  // reload interrupt after each trigger of channel switch cycle
  ESP_LOGI(TAG, "Remote command: set send cycle to %d seconds",
           cfg.sendcycle * 2);
};

void set_wifichancycle(uint8_t val[]) {
  cfg.wifichancycle = val[0];
  // update channel rotation interrupt
  timerAlarmWrite(channelSwitch, cfg.wifichancycle * 10000, true);

  ESP_LOGI(TAG,
           "Remote command: set Wifi channel switch interval to %.1f seconds",
           cfg.wifichancycle / float(100));
};

void set_blescantime(uint8_t val[]) {
  cfg.blescantime = val[0];
  ESP_LOGI(TAG, "Remote command: set BLE scan time to %.1f seconds",
           cfg.blescantime / float(100));
#ifdef BLECOUNTER
  // stop & restart BLE scan task to apply new parameter
  if (cfg.blescan) {
    stop_BLEscan();
    start_BLEscan();
  }
#endif
};

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
  }
};

void set_screensaver(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set screen saver to %s ",
           val[0] ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.screensaver = 1;
    break;
  default:
    cfg.screensaver = 0;
    break;
  }
};

void set_display(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set screen to %s", val[0] ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.screenon = 1;
    break;
  default:
    cfg.screenon = 0;
    break;
  }
};

void set_gps(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set GPS mode to %s", val[0] ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.gpsmode = 1;
    break;
  default:
    cfg.gpsmode = 0;
    break;
  };
}

void set_beacon(uint8_t val[]) {
  uint8_t id = val[0];           // use first parameter as beacon storage id
  memmove(val, val + 1, 6);      // strip off storage id
  beacons[id] = macConvert(val); // store beacon MAC in array
  ESP_LOGI(TAG, "Remote command: set beacon ID#%d", id);
  //printKey("MAC", val, 6, false); // show beacon MAC
};

void set_monitor(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set beacon monitor mode to %s",
           val ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.monitormode = 1;
    break;
  default:
    cfg.monitormode = 0;
    break;
  }
};

void set_lorasf(uint8_t val[]) {
#ifdef HAS_LORA
  ESP_LOGI(TAG, "Remote command: set LoRa SF to %d", val[0]);
  switch_lora(val[0], cfg.txpower);
#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
};

void set_loraadr(uint8_t val[]) {
#ifdef HAS_LORA
  ESP_LOGI(TAG, "Remote command: set LoRa ADR mode to %s",
           val[0] ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.adrmode = 1;
    break;
  default:
    cfg.adrmode = 0;
    break;
  }
  LMIC_setAdrMode(cfg.adrmode);
#else
  ESP_LOGW(TAG, "Remote command: LoRa not implemented");
#endif // HAS_LORA
};

void set_blescan(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set BLE scanner to %s", val[0] ? "on" : "off");
  switch (val[0]) {
  case 0:
    cfg.blescan = 0;
    macs_ble = 0; // clear BLE counter
#ifdef BLECOUNTER
    stop_BLEscan();
#endif
    break;
  default:
    cfg.blescan = 1;
#ifdef BLECOUNTER
    start_BLEscan();
#endif
    break;
  }
};

void set_wifiant(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set Wifi antenna to %s",
           val[0] ? "external" : "internal");
  switch (val[0]) {
  case 1:
    cfg.wifiant = 1;
    break;
  default:
    cfg.wifiant = 0;
    break;
  }
#ifdef HAS_ANTENNA_SWITCH
  antenna_select(cfg.wifiant);
#endif
};

void set_vendorfilter(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: set vendorfilter mode to %s",
           val[0] ? "on" : "off");
  switch (val[0]) {
  case 1:
    cfg.vendorfilter = 1;
    break;
  default:
    cfg.vendorfilter = 0;
    break;
  }
};

void set_rgblum(uint8_t val[]) {
  // Avoid wrong parameters
  cfg.rgblum = (val[0] >= 0 && val[0] <= 100) ? (uint8_t)val[0] : RGBLUMINOSITY;
  ESP_LOGI(TAG, "Remote command: set RGB Led luminosity %d", cfg.rgblum);
};

void set_lorapower(uint8_t val[]) {
#ifdef HAS_LORA
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
  senddata(CONFIGPORT);
};

void get_status(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get device status");
#ifdef HAS_BATTERY_PROBE
  uint16_t voltage = read_voltage();
#else
  uint16_t voltage = 0;
#endif
  payload.reset();
  payload.addStatus(voltage, uptime() / 1000, temperatureRead());
  senddata(STATUSPORT);
};

void get_gps(uint8_t val[]) {
  ESP_LOGI(TAG, "Remote command: get gps status");
#ifdef HAS_GPS
  gps_read();
  payload.reset();
  payload.addGPS(gps_status);
  senddata(GPSPORT);
#else
  ESP_LOGW(TAG, "GPS function not supported");
#endif
};

// assign previously defined functions to set of numeric remote commands
// format: opcode, function, #bytes params,
// flag (1 = do make settings persistent / 0 = don't)
//
cmd_t table[] = {
    {0x01, set_rssi, 1, true},          {0x02, set_countmode, 1, true},
    {0x03, set_gps, 1, true},           {0x04, set_display, 1, true},
    {0x05, set_lorasf, 1, true},        {0x06, set_lorapower, 1, true},
    {0x07, set_loraadr, 1, true},       {0x08, set_screensaver, 1, true},
    {0x09, set_reset, 1, false},        {0x0a, set_sendcycle, 1, true},
    {0x0b, set_wifichancycle, 1, true}, {0x0c, set_blescantime, 1, true},
    {0x0d, set_vendorfilter, 1, false}, {0x0e, set_blescan, 1, true},
    {0x0f, set_wifiant, 1, true},       {0x10, set_rgblum, 1, true},
    {0x11, set_monitor, 1, true},       {0x12, set_beacon, 7, false},
    {0x80, get_config, 0, false},       {0x81, get_status, 0, false},
    {0x84, get_gps, 0, false}};

// check and execute remote command
void rcommand(uint8_t cmd[], uint8_t cmdlength) {

  if (cmdlength == 0)
    return;
  else
    cmdlength--; // minus 1 byte for opcode

  int i =
      sizeof(table) / sizeof(table[0]); // number of commands in command table

  while (i--) {
    if ((cmd[0] == table[i].opcode) &&
        (table[i].params == cmdlength)) { // lookup command in opcode table
      memmove(cmd, cmd + 1,
              cmdlength); // strip opcode
      table[i].func(cmd); // execute assigned function with given parameters
      if (table[i].store) // ceck if function needs to store configuration
        saveConfig();
      break; // exit while loop, command was found
    }        // lookup command
  }          // while

} // rcommand()